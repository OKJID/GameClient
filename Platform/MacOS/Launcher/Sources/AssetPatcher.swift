import Foundation

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

    static let patchURL = "https://github.com/Okladnoj/GeneralsOnline-MacPatch/releases/latest/download/GO_Mac_Patch.zip"

    private static let patchMarkerFiles = [
        "310_ExpandedLANLobbyMenu.big",
        "340_ControlBarPro1080ZH.big",
        "340_ControlBarProHideIpZH.big",
        "340_ControlBarProHideMailZH.big",
        "340_ControlBarProZH.big",
        "400_ControlBarHDBaseZH.big",
        "400_ControlBarHDEnglishZH.big",
        "990_DecalsZH.big"
    ]

    private static let patchLocaleMarkerFiles = [
        "Data/English/generals.csf",
        "Data/German/generals.csf",
        "Data/French/generals.csf",
        "Data/Spanish/generals.csf",
        "Data/Italian/generals.csf",
        "Data/Korean/generals.csf",
        "Data/Chinese/generals.csf",
        "Data/Polish/generals.csf",
        "Data/Brazilian/generals.csf",
        "Data/Russian/generals.csf",
        "Data/Ukrainian/generals.csf"
    ]

    static let availableLanguages = [
        "english", "german", "french", "spanish", "italian",
        "korean", "chinese", "polish", "brazilian",
        "russian", "ukrainian"
    ]

    private static let conflictingEAFiles = [
        "PatchData.big",
        "PatchINI.big",
        "PatchWindow.big",
        "PatchZH.big"
    ]

    // MARK: - Public API

    func isCommunityPatchInstalled(at zhDir: URL) -> Bool {
        let fm = FileManager.default
        let allMarkers = Self.patchMarkerFiles + Self.patchLocaleMarkerFiles
        return allMarkers.allSatisfy { marker in
            fm.fileExists(atPath: zhDir.appendingPathComponent(marker).path)
        }
    }

    static func installedLanguages(at zhDir: URL) -> [String] {
        let fm = FileManager.default
        return patchLocaleMarkerFiles.compactMap { relativePath in
            let fullPath = zhDir.appendingPathComponent(relativePath).path
            guard fm.fileExists(atPath: fullPath) else { return nil }

            let components = relativePath.split(separator: "/")
            guard components.count >= 2 else { return nil }
            return String(components[1]).lowercased()
        }
    }

    static func findZHDirectory(at rootURL: URL) -> URL? {
        let fm = FileManager.default
        guard let items = try? fm.contentsOfDirectory(
            at: rootURL,
            includingPropertiesForKeys: [.isDirectoryKey],
            options: .skipsHiddenFiles
        ) else {
            return nil
        }

        for itemURL in items {
            guard let isDir = try? itemURL.resourceValues(forKeys: [.isDirectoryKey]).isDirectory, isDir else { continue }
            guard let subItems = try? fm.contentsOfDirectory(atPath: itemURL.path) else { continue }

            if subItems.contains(where: { $0.lowercased() == "inizh.big" }) {
                return itemURL
            }
        }

        return nil
    }

    func startPatching(rootDir: URL, zhDir: URL) {
        guard !state.isRunning else { return }

        consoleLog = ""
        appendLog("[*] Starting patch process...\n")
        appendLog("[*] ZH directory: \(zhDir.path)\n")

        state = .cleaning
        cleanEAPatchFiles(in: zhDir, rootDir: rootDir)
        downloadAndApplyPatch(zhDir: zhDir)
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

    private func downloadAndApplyPatch(zhDir: URL) {
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
                    self.applyPatch(from: tempZipURL, zhDir: zhDir)
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

    private func applyPatch(from zipURL: URL, zhDir: URL) {
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
                    self.movePatchFiles(from: tempExtractDir, to: zhDir)
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

    private func movePatchFiles(from extractDir: URL, to zhDir: URL) {
        let fm = FileManager.default

        var sourceDir = extractDir
        let assetsSubdir = extractDir.appendingPathComponent("Assets")
        if fm.fileExists(atPath: assetsSubdir.path) {
            sourceDir = assetsSubdir
        }

        do {
            try mergeDirectory(from: sourceDir, to: zhDir, fm: fm)
            try? fm.removeItem(at: extractDir)

            appendLog("[✓] Community Patch successfully applied!\n")
            state = .completed
        } catch {
            try? fm.removeItem(at: extractDir)
            fail("Failed to move patch files: \(error.localizedDescription)")
        }
    }

    private func mergeDirectory(from src: URL, to dst: URL, fm: FileManager) throws {
        if !fm.fileExists(atPath: dst.path) {
            try fm.createDirectory(at: dst, withIntermediateDirectories: true)
        }

        let items = try fm.contentsOfDirectory(atPath: src.path)
        for item in items {
            let srcItem = src.appendingPathComponent(item)
            let dstItem = dst.appendingPathComponent(item)

            var isDir: ObjCBool = false
            fm.fileExists(atPath: srcItem.path, isDirectory: &isDir)

            if isDir.boolValue {
                try mergeDirectory(from: srcItem, to: dstItem, fm: fm)
            } else {
                if fm.fileExists(atPath: dstItem.path) {
                    try fm.removeItem(at: dstItem)
                }
                try fm.moveItem(at: srcItem, to: dstItem)
            }
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
        DispatchQueue.main.async {
            self.state = .failed(message)
            self.appendLog("\n[✗] \(message)\n")
        }
    }
}
