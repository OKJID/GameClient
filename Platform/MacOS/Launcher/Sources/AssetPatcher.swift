import Foundation

struct PatchTarget {
    let profile: GameProfile
    let directory: URL
}

enum PatchState: Equatable {
    case idle
    case cleaning
    case downloadingPatch(progress: Double)
    case unpacking
    case completed
    case failed(String)

    var isRunning: Bool {
        switch self {
        case .cleaning, .downloadingPatch, .unpacking:
            return true
        default:
            return false
        }
    }

    var statusText: String {
        switch self {
        case .idle: return L10n.patch.status.idle
        case .cleaning: return L10n.patch.status.cleaning
        case .downloadingPatch(let progress): return String(format: L10n.patch.status.downloading, progress * 100)
        case .unpacking: return L10n.patch.status.unpacking
        case .completed: return L10n.patch.status.completed
        case .failed(let msg): return L10n.patch.status.error.replacingOccurrences(of: "%@", with: msg)
        }
    }
}

class AssetPatcher: ObservableObject {
    @Published var state: PatchState = .idle
    @Published var consoleLog: String = ""

    private var downloadObservation: NSKeyValueObservation?
    private var startedAt: Date?

    private var elapsedSeconds: Double {
        guard let startedAt else { return 0 }
        return Date().timeIntervalSince(startedAt)
    }

    static let patchURL = "https://github.com/Okladnoj/GeneralsOnline-MacPatch/releases/latest/download/GO_Mac_Patch.zip"

    static let mapsDirName = "Maps"

    private static let conflictingEAFiles = [
        "PatchData.big",
        "PatchINI.big",
        "PatchWindow.big",
        "PatchZH.big"
    ]

    // MARK: - Public API

    func isCommunityPatchInstalled(_ profile: GameProfile, at directory: URL) -> Bool {
        profile.isPatchInstalled(at: directory)
    }

    func startPatching(rootDir: URL, targets: [PatchTarget]) {
        guard !state.isRunning else { return }
        guard !targets.isEmpty else {
            fail("No game installation found to patch.")
            return
        }

        consoleLog = ""
        startedAt = Date()
        appendLog("[*] Starting patch process...\n")
        for target in targets {
            appendLog("[*] \(target.profile.displayName): \(target.directory.path)\n")
        }

        state = .cleaning
        for target in targets where target.profile.id == .zeroHour {
            cleanEAPatchFiles(in: target.directory, rootDir: rootDir)
        }

        downloadAndApplyPatch(targets: targets)
    }

    // MARK: - Clean

    private func cleanEAPatchFiles(in zhDir: URL, rootDir: URL) {
        let fm = FileManager.default

        for file in Self.conflictingEAFiles {
            let zhPath = zhDir.appendingPathComponent(file)
            if fm.fileExists(atPath: zhPath.path) {
                try? fm.removeItem(at: zhPath)
                appendLog("[*] Removed conflicting file: \(file)\n")
            }

            let rootPath = rootDir.appendingPathComponent(file)
            if fm.fileExists(atPath: rootPath.path) {
                try? fm.removeItem(at: rootPath)
                appendLog("[*] Removed conflicting file from root: \(file)\n")
            }
        }
    }

    // MARK: - Download

    private func downloadAndApplyPatch(targets: [PatchTarget]) {
        DispatchQueue.main.async {
            self.state = .downloadingPatch(progress: 0.0)
            self.appendLog("\n[⭳] Downloading Community Patch & Maps...\n")
        }

        guard let patchURL = URL(string: Self.patchURL) else {
            fail("Invalid patch URL.")
            return
        }

        let task = URLSession.shared.downloadTask(with: patchURL) { [weak self] localURL, _, error in
            guard let self else { return }

            if let error {
                self.fail("Failed to download patch: \(error.localizedDescription)")
                return
            }

            guard let localURL else {
                self.fail("Patch download failed: No file URL returned.")
                return
            }

            let tempZipURL = FileManager.default.temporaryDirectory
                .appendingPathComponent("go_patch_\(UUID().uuidString).zip")
            try? FileManager.default.removeItem(at: tempZipURL)

            do {
                try FileManager.default.moveItem(at: localURL, to: tempZipURL)
                DispatchQueue.main.async {
                    self.downloadObservation?.invalidate()
                    self.downloadObservation = nil
                    self.applyPatch(from: tempZipURL, targets: targets)
                }
            } catch {
                self.fail("Failed to prepare patch file: \(error.localizedDescription)")
            }
        }

        downloadObservation = task.progress.observe(\.fractionCompleted) { [weak self] progress, _ in
            let fraction = progress.fractionCompleted
            DispatchQueue.main.async {
                self?.state = .downloadingPatch(progress: fraction)
            }
        }

        task.resume()
    }

    // MARK: - Unpack

    private func applyPatch(from zipURL: URL, targets: [PatchTarget]) {
        state = .unpacking
        appendLog("[*] Unpacking patch...\n")

        let tempExtractDir = FileManager.default.temporaryDirectory
            .appendingPathComponent("go_patch_extract_\(UUID().uuidString)")
        try? FileManager.default.createDirectory(at: tempExtractDir, withIntermediateDirectories: true)

        let process = Process()
        process.executableURL = URL(fileURLWithPath: "/usr/bin/unzip")
        process.arguments = ["-o", zipURL.path, "-d", tempExtractDir.path]

        process.terminationHandler = { [weak self] proc in
            DispatchQueue.main.async {
                guard let self else { return }
                try? FileManager.default.removeItem(at: zipURL)

                if proc.terminationStatus == 0 {
                    self.movePatchFiles(from: tempExtractDir, targets: targets)
                } else {
                    try? FileManager.default.removeItem(at: tempExtractDir)
                    self.fail("Failed to unpack patch (unzip exited with code \(proc.terminationStatus))")
                }
            }
        }

        do {
            try process.run()
        } catch {
            try? FileManager.default.removeItem(at: tempExtractDir)
            fail("Failed to execute unzip: \(error.localizedDescription)")
        }
    }

    private func movePatchFiles(from extractDir: URL, targets: [PatchTarget]) {
        let fm = FileManager.default

        do {
            let applied = try Self.applyExtractedPatch(
                from: extractDir,
                targets: targets,
                log: { [weak self] text in self?.appendLog(text) }
            )

            try? fm.removeItem(at: extractDir)

            guard applied > 0 else {
                fail("Patch archive carried no assets for the installed games.")
                return
            }

            appendLog("[✓] Community Patch successfully applied!\n")
            Analytics.logPatchFinished(seconds: elapsedSeconds)
            state = .completed
        } catch {
            try? fm.removeItem(at: extractDir)
            fail("Failed to move patch files: \(error.localizedDescription)")
        }
    }

    static func applyExtractedPatch(
        from extractDir: URL,
        targets: [PatchTarget],
        log: ((String) -> Void)? = nil
    ) throws -> Int {
        let fm = FileManager.default
        var applied = 0

        for target in targets {
            let sourceDir = extractDir.appendingPathComponent(target.profile.patchAssetsDirName)
            guard fm.fileExists(atPath: sourceDir.path) else {
                log?("[*] \(target.profile.displayName): nothing in this patch, skipped\n")
                continue
            }

            let mapNames = (try? fm.contentsOfDirectory(atPath: sourceDir.appendingPathComponent(mapsDirName).path)) ?? []

            let userMaps = target.profile.userDataDirURL.appendingPathComponent(mapsDirName)
            try merge(from: sourceDir, to: target.directory, fm: fm, log: log, redirectMapsTo: userMaps)

            dropInstalledMaps(named: mapNames, from: target.directory.appendingPathComponent(mapsDirName), fm: fm, log: log)
            try PatchVersions.markCurrent(at: target.directory)

            log?("[✓] \(target.profile.displayName): assets applied\n")
            applied += 1
        }

        return applied
    }

    static func dropInstalledMaps(named names: [String], from installMaps: URL, fm: FileManager, log: ((String) -> Void)?) {
        guard fm.fileExists(atPath: installMaps.path) else { return }

        var dropped = 0
        for name in names {
            let stale = installMaps.appendingPathComponent(name)
            guard fm.fileExists(atPath: stale.path) else { continue }
            guard (try? fm.removeItem(at: stale)) != nil else { continue }

            dropped += 1
        }

        guard dropped > 0 else { return }

        log?("[*] Removed \(dropped) map(s) left in \(installMaps.path)\n")
    }

    static func merge(
        from src: URL,
        to dst: URL,
        fm: FileManager,
        log: ((String) -> Void)?,
        redirectMapsTo userMapsDir: URL? = nil
    ) throws {
        if !fm.fileExists(atPath: dst.path) {
            try fm.createDirectory(at: dst, withIntermediateDirectories: true)
        }

        for item in try fm.contentsOfDirectory(atPath: src.path) {
            let srcItem = src.appendingPathComponent(item)
            let dstItem = dst.appendingPathComponent(item)

            var isDir: ObjCBool = false
            fm.fileExists(atPath: srcItem.path, isDirectory: &isDir)

            if isDir.boolValue {
                if let userMapsDir, item.compare(mapsDirName, options: .caseInsensitive) == .orderedSame {
                    log?("[*] Maps → \(userMapsDir.path)\n")
                    try merge(from: srcItem, to: userMapsDir, fm: fm, log: log)
                    continue
                }

                try merge(from: srcItem, to: dstItem, fm: fm, log: log)
                continue
            }

            if fm.fileExists(atPath: dstItem.path) {
                log?("[*] Replacing \(dstItem.lastPathComponent)\n")
                try fm.removeItem(at: dstItem)
            }

            try fm.moveItem(at: srcItem, to: dstItem)
        }
    }

    // MARK: - Logging

    func appendLog(_ text: String) {
        print(text, terminator: "")
        DispatchQueue.main.async {
            self.consoleLog += text
        }
    }

    private func fail(_ message: String) {
        Analytics.logPatchFailed(reason: message)

        DispatchQueue.main.async {
            self.state = .failed(message)
            self.appendLog("\n[✗] \(message)\n")
        }
    }
}
