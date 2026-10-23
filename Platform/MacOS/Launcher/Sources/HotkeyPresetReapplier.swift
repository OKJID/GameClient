import Foundation

struct HotkeyReapplyJob {
    let profile: GameProfile
    let files: HotkeyFiles
    let launch: HotkeyEngineLaunch
    let installRoot: URL
}

final class HotkeyPresetReapplier {
    private var queue: [HotkeyReapplyJob] = []
    private var isRunning = false

    func sync(_ jobs: [HotkeyReapplyJob]) {
        queue = jobs.filter(Self.needsReapply)
        guard !isRunning else { return }

        runNext()
    }

    private static func needsReapply(_ job: HotkeyReapplyJob) -> Bool {
        guard let state = job.files.loadPresetState() else { return false }
        return !state.belongs(to: job.installRoot.path)
    }

    private func runNext() {
        guard !queue.isEmpty else {
            isRunning = false
            return
        }

        isRunning = true
        reapply(queue.removeFirst()) { [weak self] in
            self?.runNext()
        }
    }

    private func reapply(_ job: HotkeyReapplyJob, completion: @escaping () -> Void) {
        if let dump = job.files.loadDump() {
            job.files.restoreOriginals(of: dump, root: job.installRoot)
        }

        guard let name = job.files.loadPresetState()?.name,
              let entry = HotkeyPresetStore.entry(named: name, in: job.profile.hotkeyPresetsDirURL) else {
            job.files.savePresetState(HotkeyPresetState(name: nil, isModified: false, root: job.installRoot.path))
            completion()
            return
        }

        readPreset(entry, job: job) { preset in
            guard let preset else {
                completion()
                return
            }

            Self.apply(preset, named: name, job: job, completion: completion)
        }
    }

    private func readPreset(_ entry: HotkeyPresetEntry, job: HotkeyReapplyJob, completion: @escaping (HotkeyPreset?) -> Void) {
        if case .shipped(let preset) = entry.kind {
            completion(preset)
            return
        }

        HotkeyEngineRunner.importPreset(job.launch, from: entry.url, to: job.files.importURL) { isPresetFresh in
            let preset = isPresetFresh ? HotkeyPresetStore.load(job.files.importURL) : nil
            try? FileManager.default.removeItem(at: job.files.importURL)
            completion(preset)
        }
    }

    private static func apply(_ preset: HotkeyPreset, named name: String, job: HotkeyReapplyJob, completion: @escaping () -> Void) {
        guard job.files.saveReapplyEdits(HotkeyPresetMatcher.edits(of: preset)) else {
            completion()
            return
        }

        HotkeyEngineRunner.apply(job.launch, edits: job.files.reapplyURL, files: job.files) { isDumpFresh in
            try? FileManager.default.removeItem(at: job.files.reapplyURL)
            if isDumpFresh {
                job.files.savePresetState(HotkeyPresetState(name: name, isModified: false, root: job.installRoot.path))
            }
            completion()
        }
    }
}
