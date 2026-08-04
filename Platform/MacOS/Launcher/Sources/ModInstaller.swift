import Foundation

enum ModInstallState: Equatable {
    case idle
    case downloading(part: Int, total: Int, progress: Double)
    case unpacking(part: Int, total: Int)
    case removing
    case completed
    case failed(String)

    var isRunning: Bool {
        switch self {
        case .downloading, .unpacking, .removing: return true
        default: return false
        }
    }

    var fraction: Double {
        switch self {
        case .idle, .failed, .removing: return 0
        case .completed: return 1
        case .downloading(let part, let total, let progress):
            return (Double(part - 1) + progress * 0.9) / Double(total)
        case .unpacking(let part, let total):
            return (Double(part - 1) + 0.95) / Double(total)
        }
    }

    var statusText: String {
        switch self {
        case .idle:
            return L10n.mod.status.notInstalled
        case .downloading(let part, let total, let progress):
            let percent = Int(progress * 100)
            return total > 1
                ? String(format: L10n.mod.status.downloadingPart, part, total, percent)
                : String(format: L10n.mod.status.downloading, percent)
        case .unpacking(let part, let total):
            return total > 1
                ? String(format: L10n.mod.status.unpackingPart, part, total)
                : L10n.mod.status.unpacking
        case .removing:
            return L10n.mod.status.removing
        case .completed:
            return L10n.mod.status.installed
        case .failed(let message):
            return String(format: L10n.mod.status.error, message)
        }
    }
}

struct ModJob: Equatable {
    let id: GameID
    let root: URL
    var state: ModInstallState
}

class ModInstaller: ObservableObject {
    @Published private(set) var job: ModJob?
    @Published private(set) var consoleLog: String = ""

    private var downloadObservation: NSKeyValueObservation?
    private var startedAt: Date?
    private var stage: String = "idle"

    var isBusy: Bool {
        job?.state.isRunning ?? false
    }

    var runningModID: GameID? {
        guard let job = job, job.state.isRunning else { return nil }
        return job.id
    }

    func state(for id: GameID, in root: URL) -> ModInstallState {
        guard let job = job, job.id == id, job.root == root else { return .idle }
        return job.state
    }

    func install(_ profile: GameProfile, installRoot: URL) {
        guard let mod = profile.mod, let destination = profile.modDirectory(installRoot: installRoot) else {
            return
        }
        guard !isBusy else { return }

        consoleLog = ""
        appendLog("[*] Installing \(profile.displayName) into \(destination.path)\n")
        job = ModJob(
            id: profile.id,
            root: installRoot,
            state: .downloading(part: 1, total: mod.partCount, progress: 0)
        )

        startedAt = Date()
        stage = "prepare"
        Analytics.logModInstallStarted(profile.id, parts: mod.partCount)

        do {
            try recreateDirectory(destination)
        } catch {
            fail(profile, destination: nil, message: "Cannot prepare mod directory: \(error.localizedDescription)")
            return
        }

        downloadPart(0, profile: profile, mod: mod, destination: destination)
    }

    func remove(_ profile: GameProfile, installRoot: URL) {
        guard let destination = profile.modDirectory(installRoot: installRoot) else { return }
        guard !isBusy else { return }

        consoleLog = ""
        appendLog("[*] Removing \(profile.displayName) from \(destination.path)\n")
        job = ModJob(id: profile.id, root: installRoot, state: .removing)

        DispatchQueue.global(qos: .userInitiated).async { [weak self] in
            try? FileManager.default.removeItem(at: destination)

            DispatchQueue.main.async {
                guard let self else { return }
                self.appendLog("[*] Removed \(profile.displayName)\n")
                Analytics.logModRemoved(profile.id)
                self.clearJob()
            }
        }
    }

    // MARK: - Download

    private func downloadPart(_ index: Int, profile: GameProfile, mod: ModSpec, destination: URL) {
        guard index < mod.partCount else {
            finish(profile, destination: destination)
            return
        }

        stage = "download_part_\(index + 1)"
        setState(.downloading(part: index + 1, total: mod.partCount, progress: 0))
        appendLog("[⭳] Part \(index + 1)/\(mod.partCount): \(mod.downloadURLs[index].lastPathComponent)\n")

        let task = URLSession.shared.downloadTask(with: mod.downloadURLs[index]) { [weak self] localURL, _, error in
            guard let self else { return }

            if let error {
                self.fail(profile, destination: destination, message: error.localizedDescription)
                return
            }

            guard let localURL else {
                self.fail(profile, destination: destination, message: "No file returned for part \(index + 1)")
                return
            }

            let tempZip = FileManager.default.temporaryDirectory
                .appendingPathComponent("go_mod_\(UUID().uuidString).zip")

            do {
                try FileManager.default.moveItem(at: localURL, to: tempZip)
            } catch {
                self.fail(profile, destination: destination, message: error.localizedDescription)
                return
            }

            DispatchQueue.main.async {
                self.downloadObservation?.invalidate()
                self.downloadObservation = nil
                self.unpackPart(index, zip: tempZip, profile: profile, mod: mod, destination: destination)
            }
        }

        downloadObservation = task.progress.observe(\.fractionCompleted) { [weak self] progress, _ in
            let fraction = progress.fractionCompleted
            DispatchQueue.main.async {
                self?.setState(.downloading(part: index + 1, total: mod.partCount, progress: fraction))
            }
        }

        task.resume()
    }

    // MARK: - Unpack

    private func unpackPart(_ index: Int, zip: URL, profile: GameProfile, mod: ModSpec, destination: URL) {
        stage = "unpack_part_\(index + 1)"
        setState(.unpacking(part: index + 1, total: mod.partCount))

        let process = Process()
        process.executableURL = URL(fileURLWithPath: "/usr/bin/unzip")
        process.arguments = ["-qo", zip.path, "-d", destination.path]

        process.terminationHandler = { [weak self] proc in
            DispatchQueue.main.async {
                guard let self else { return }
                try? FileManager.default.removeItem(at: zip)

                guard proc.terminationStatus == 0 else {
                    self.fail(
                        profile,
                        destination: destination,
                        message: "unzip failed on part \(index + 1) (code \(proc.terminationStatus))"
                    )
                    return
                }

                self.downloadPart(index + 1, profile: profile, mod: mod, destination: destination)
            }
        }

        do {
            try process.run()
        } catch {
            try? FileManager.default.removeItem(at: zip)
            fail(profile, destination: destination, message: error.localizedDescription)
        }
    }

    private func finish(_ profile: GameProfile, destination: URL) {
        stage = "verify"
        let fm = FileManager.default
        let missing = (profile.mod?.markers ?? []).filter { marker in
            !fm.fileExists(atPath: destination.appendingPathComponent(marker).path)
        }

        guard missing.isEmpty else {
            let preview = missing.prefix(3).joined(separator: ", ")
            fail(profile, destination: destination, message: "\(missing.count) file(s) missing after unpack: \(preview)")
            return
        }

        appendLog("[✓] \(profile.displayName) installed\n")
        Analytics.logModInstallFinished(profile.id, seconds: elapsedSeconds())
        clearJob()
    }

    private func elapsedSeconds() -> Double {
        guard let startedAt else { return 0 }
        return Date().timeIntervalSince(startedAt)
    }

    // MARK: - Helpers

    private func recreateDirectory(_ url: URL) throws {
        let fm = FileManager.default
        if fm.fileExists(atPath: url.path) {
            try fm.removeItem(at: url)
        }

        try fm.createDirectory(at: url, withIntermediateDirectories: true)
    }

    private func setState(_ state: ModInstallState) {
        DispatchQueue.main.async {
            self.job?.state = state
        }
    }

    private func clearJob() {
        DispatchQueue.main.async {
            self.job = nil
        }
    }

    private func fail(_ profile: GameProfile, destination: URL?, message: String) {
        if let destination {
            try? FileManager.default.removeItem(at: destination)
        }

        appendLog("\n[✗] \(profile.displayName): \(message)\n")
        Analytics.logModInstallFailed(profile.id, stage: stage, reason: message)
        setState(.failed(message))
    }

    private func appendLog(_ text: String) {
        print(text, terminator: "")
        DispatchQueue.main.async {
            self.consoleLog += text
        }
    }
}
