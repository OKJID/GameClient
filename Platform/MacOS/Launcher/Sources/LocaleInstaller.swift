import Foundation

struct LocaleJob: Equatable {
    let language: String
    var state: ModInstallState
}

enum LocalePackStatus: Equatable {
    case hidden
    case native
    case available(LocalePack)
    case outdated(LocalePack)
    case installed(LocalePack)
    case running(ModInstallState)
    case failed(String, LocalePack)
}

final class LocaleInstaller: ObservableObject {
    @Published private(set) var job: LocaleJob?

    private var downloadObservation: NSKeyValueObservation?

    var isBusy: Bool {
        job?.state.isRunning ?? false
    }

    func state(for language: String) -> ModInstallState {
        guard let job, job.language == language else { return .idle }
        return job.state
    }

    func install(_ pack: LocalePack, targets: [PatchTarget]) {
        guard !isBusy, !targets.isEmpty else { return }

        let workDir = FileManager.default.temporaryDirectory
            .appendingPathComponent("go_locale_\(UUID().uuidString)", isDirectory: true)
        job = LocaleJob(language: pack.id, state: .downloading(part: 1, total: pack.partCount, progress: 0))

        do {
            try FileManager.default.createDirectory(at: workDir, withIntermediateDirectories: true)
        } catch {
            fail(workDir: nil, message: error.localizedDescription)
            return
        }

        downloadPart(0, pack: pack, targets: targets, workDir: workDir)
    }

    func remove(language: String, targets: [PatchTarget]) {
        guard !isBusy else { return }

        job = LocaleJob(language: language, state: .removing)
        DispatchQueue.global(qos: .userInitiated).async { [weak self] in
            targets.forEach { Self.removeInstalledFiles(language: language, in: $0.directory) }

            DispatchQueue.main.async {
                self?.job = nil
            }
        }
    }

    // MARK: - Download

    private func downloadPart(_ index: Int, pack: LocalePack, targets: [PatchTarget], workDir: URL) {
        guard index < pack.partCount else {
            apply(pack, targets: targets, workDir: workDir)
            return
        }

        setState(.downloading(part: index + 1, total: pack.partCount, progress: 0))

        let task = URLSession.shared.downloadTask(with: pack.downloadURLs[index]) { [weak self] localURL, _, error in
            guard let self else { return }

            if let error {
                self.fail(workDir: workDir, message: error.localizedDescription)
                return
            }

            guard let localURL else {
                self.fail(workDir: workDir, message: "No file returned for part \(index + 1)")
                return
            }

            let zip = workDir.appendingPathComponent("part\(index + 1).zip")
            do {
                try FileManager.default.moveItem(at: localURL, to: zip)
            } catch {
                self.fail(workDir: workDir, message: error.localizedDescription)
                return
            }

            DispatchQueue.main.async {
                self.downloadObservation?.invalidate()
                self.downloadObservation = nil
                self.unpackPart(index, zip: zip, pack: pack, targets: targets, workDir: workDir)
            }
        }

        downloadObservation = task.progress.observe(\.fractionCompleted) { [weak self] progress, _ in
            let fraction = progress.fractionCompleted
            DispatchQueue.main.async {
                self?.setState(.downloading(part: index + 1, total: pack.partCount, progress: fraction))
            }
        }

        task.resume()
    }

    // MARK: - Unpack

    private func unpackPart(_ index: Int, zip: URL, pack: LocalePack, targets: [PatchTarget], workDir: URL) {
        setState(.unpacking(part: index + 1, total: pack.partCount))

        let process = Process()
        process.executableURL = URL(fileURLWithPath: "/usr/bin/unzip")
        process.arguments = ["-qo", zip.path, "-d", Self.extractedDir(in: workDir).path]

        process.terminationHandler = { [weak self] proc in
            DispatchQueue.main.async {
                guard let self else { return }
                try? FileManager.default.removeItem(at: zip)

                guard proc.terminationStatus == 0 else {
                    self.fail(workDir: workDir, message: "unzip failed on part \(index + 1) (code \(proc.terminationStatus))")
                    return
                }

                self.downloadPart(index + 1, pack: pack, targets: targets, workDir: workDir)
            }
        }

        do {
            try process.run()
        } catch {
            fail(workDir: workDir, message: error.localizedDescription)
        }
    }

    // MARK: - Apply

    private func apply(_ pack: LocalePack, targets: [PatchTarget], workDir: URL) {
        setState(.unpacking(part: pack.partCount, total: pack.partCount))

        DispatchQueue.global(qos: .userInitiated).async { [weak self] in
            let failure = Self.place(pack, from: Self.extractedDir(in: workDir), into: targets)
                ?? Self.missingMarkers(of: pack, in: targets)

            DispatchQueue.main.async {
                guard let self else { return }
                guard let failure else {
                    try? FileManager.default.removeItem(at: workDir)
                    self.job = nil
                    return
                }

                targets.forEach { Self.removeInstalledFiles(language: pack.id, in: $0.directory) }
                self.fail(workDir: workDir, message: failure)
            }
        }
    }

    private static func place(_ pack: LocalePack, from extracted: URL, into targets: [PatchTarget]) -> String? {
        for target in targets {
            let source = extracted.appendingPathComponent(target.profile.patchAssetsDirName)
            guard FileManager.default.fileExists(atPath: source.path) else { continue }

            removeInstalledFiles(language: pack.id, in: target.directory)
            if let failure = move(pack, from: source, to: target.directory) {
                return failure
            }
        }

        return nil
    }

    private static func move(_ pack: LocalePack, from source: URL, to directory: URL) -> String? {
        let fm = FileManager.default
        var placed: [String] = []
        defer {
            try? LocaleManifest(id: pack.id, packageVersion: pack.packageVersion, files: placed).write(in: directory)
        }

        for relative in relativeFiles(under: source) {
            let destination = directory.appendingPathComponent(relative)
            do {
                try fm.createDirectory(at: destination.deletingLastPathComponent(), withIntermediateDirectories: true)
                if fm.fileExists(atPath: destination.path) {
                    try fm.removeItem(at: destination)
                }
                try fm.moveItem(at: source.appendingPathComponent(relative), to: destination)
                placed.append(relative)
            } catch {
                return "\(relative): \(error.localizedDescription)"
            }
        }

        return nil
    }

    private static func missingMarkers(of pack: LocalePack, in targets: [PatchTarget]) -> String? {
        let missing = pack.markers.filter { marker in
            guard let url = LocaleLayout.installedURL(for: marker, targets: targets) else { return false }
            return !FileManager.default.fileExists(atPath: url.path)
        }

        guard !missing.isEmpty else { return nil }
        return "\(missing.count) file(s) missing after unpack: \(missing.prefix(3).joined(separator: ", "))"
    }

    // MARK: - Removal

    static func removeInstalledFiles(language: String, in directory: URL) {
        guard let manifest = LocaleManifest.read(language: language, in: directory) else { return }

        let fm = FileManager.default
        for relative in manifest.files {
            let file = directory.appendingPathComponent(relative)
            try? fm.removeItem(at: file)
            pruneEmptyParents(of: file, below: directory)
        }

        try? fm.removeItem(at: LocaleLayout.manifestURL(language: language, in: directory))
    }

    private static func pruneEmptyParents(of file: URL, below root: URL) {
        let fm = FileManager.default
        var directory = file.deletingLastPathComponent()
        while directory.path.count > root.path.count,
              let entries = try? fm.contentsOfDirectory(atPath: directory.path),
              entries.allSatisfy({ $0 == ".DS_Store" }) {
            try? fm.removeItem(at: directory)
            directory = directory.deletingLastPathComponent()
        }
    }

    // MARK: - Helpers

    private static func extractedDir(in workDir: URL) -> URL {
        workDir.appendingPathComponent("extracted", isDirectory: true)
    }

    private static func relativeFiles(under root: URL) -> [String] {
        guard let walker = FileManager.default.enumerator(
            at: root,
            includingPropertiesForKeys: [.isRegularFileKey],
            options: [.skipsHiddenFiles]
        ) else {
            return []
        }

        let rootPath = root.standardizedFileURL.path + "/"
        return walker.compactMap { item -> String? in
            guard let url = item as? URL,
                  (try? url.resourceValues(forKeys: [.isRegularFileKey]).isRegularFile) == true else {
                return nil
            }

            return String(url.standardizedFileURL.path.dropFirst(rootPath.count))
        }
    }

    private func setState(_ state: ModInstallState) {
        DispatchQueue.main.async {
            self.job?.state = state
        }
    }

    private func fail(workDir: URL?, message: String) {
        if let workDir {
            try? FileManager.default.removeItem(at: workDir)
        }

        setState(.failed(message))
    }
}
