import Foundation
import AppKit
import Security

enum SteamCMDState: Equatable {
    case idle
    case installingRosetta
    case downloadingSteamCMD
    case waitingForCredentials
    case authenticating
    case waitingSteamGuard
    case downloading(progress: String)
    case validating
    case downloadingPatch(progress: Double)
    case unpackingPatch
    case completed
    case failed(String)

    var isRunning: Bool {
        switch self {
        case .installingRosetta, .downloadingSteamCMD, .authenticating, .downloading, .validating, .downloadingPatch, .unpackingPatch:
            return true
        default:
            return false
        }
    }

    var analyticsStage: String {
        switch self {
        case .idle: return "idle"
        case .installingRosetta: return "installing_rosetta"
        case .downloadingSteamCMD: return "installing_steamcmd"
        case .waitingForCredentials: return "awaiting_credentials"
        case .authenticating: return "authenticating"
        case .waitingSteamGuard: return "steam_guard"
        case .downloading: return "downloading"
        case .validating: return "validating"
        case .downloadingPatch: return "downloading_patch"
        case .unpackingPatch: return "unpacking_patch"
        case .completed: return "completed"
        case .failed: return "failed"
        }
    }

    var statusText: String {
        switch self {
        case .idle: return L10n.steam.status.ready
        case .installingRosetta: return L10n.steam.status.installingRosetta
        case .downloadingSteamCMD: return L10n.steam.status.installingSteamCMD
        case .waitingForCredentials: return L10n.steam.status.awaitingCreds
        case .authenticating: return L10n.steam.status.authenticating
        case .waitingSteamGuard: return L10n.steam.status.steamGuard
        case .downloading(let progress): return L10n.steam.status.downloading.replacingOccurrences(of: "%@", with: progress)
        case .validating: return L10n.steam.status.validating
        case .downloadingPatch(let progress): return String(format: L10n.steam.status.downloadingPatch, progress * 100)
        case .unpackingPatch: return L10n.steam.status.unpacking
        case .completed: return L10n.steam.status.assetsReady
        case .failed(let msg): return L10n.steam.status.error.replacingOccurrences(of: "%@", with: msg)
        }
    }
}

struct SteamCredentials {
    let username: String
    let password: String
}

class SteamCMDManager: ObservableObject {
    @Published var state: SteamCMDState = .idle
    @Published var consoleLog: String = ""
    @Published var steamGuardCode: String = ""
    @Published var showPurchaseAlert: Bool = false
    @Published var showRosettaAlert: Bool = false

    var lastUsername: String = ""

    static let appID = "2732960"

    private var process: Process?
    private var inputPipe: Pipe?
    private var downloadObservation: NSKeyValueObservation?
    private var startedAt: Date?
    private var pendingCredentials: SteamCredentials?

    private var elapsedSeconds: Double {
        guard let startedAt else { return 0 }
        return Date().timeIntervalSince(startedAt)
    }

    var supportDir: URL {
        let base = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask).first!
        return base.appendingPathComponent("Generals Online")
    }

    var steamCMDDir: URL { supportDir.appendingPathComponent("steamcmd") }
    var steamCMDBinary: URL { steamCMDDir.appendingPathComponent("steamcmd") }
    var installDir: URL {
        let docs = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first!
        return docs.appendingPathComponent("Generals Online")
    }

    var assetsDir: URL { installDir.appendingPathComponent("Assets") }
    var baseGameDir: URL { installDir.appendingPathComponent("ZH_Generals") }

    var patchTargets: [PatchTarget] {
        GameProfile.baseGames.compactMap { profile in
            guard let directory = [assetsDir, baseGameDir].first(where: { profile.matchesInstall(at: $0) }) else {
                return nil
            }

            return PatchTarget(profile: profile, directory: directory)
        }
    }

    var isSteamCMDInstalled: Bool {
        FileManager.default.fileExists(atPath: steamCMDBinary.path)
    }

    var areAssetsValid: Bool {
        let fm = FileManager.default
        let zhFiles = (try? fm.contentsOfDirectory(atPath: assetsDir.path)) ?? []
        let baseFiles = (try? fm.contentsOfDirectory(atPath: baseGameDir.path)) ?? []

        let hasZH = zhFiles.contains { $0.lowercased() == "inizh.big" }
        let hasBase = baseFiles.contains { $0.lowercased() == "ini.big" } || zhFiles.contains { $0.lowercased() == "ini.big" }

        return hasZH && hasBase
    }

    private func reorganizeAssets() {
        let fm = FileManager.default
        let source = assetsDir.appendingPathComponent("ZH_Generals")
        let destination = baseGameDir

        guard fm.fileExists(atPath: source.path) else {
            appendLog("[*] ZH_Generals already in correct location\n")
            return
        }

        if fm.fileExists(atPath: destination.path) {
            try? fm.removeItem(at: destination)
        }

        do {
            try fm.moveItem(at: source, to: destination)
            appendLog("[✓] Moved ZH_Generals/ alongside Assets/\n")
        } catch {
            appendLog("[!] Failed to move ZH_Generals: \(error.localizedDescription)\n")
        }
    }

    private func cleanEAPatchFiles() {
        let filesToRemove = ["PatchData.big", "PatchINI.big", "PatchWindow.big", "PatchZH.big"]
        let fm = FileManager.default
        
        for file in filesToRemove {
            let path = assetsDir.appendingPathComponent(file)
            if fm.fileExists(atPath: path.path) {
                try? fm.removeItem(at: path)
                appendLog("[*] Removed EA modern patch file: \(file)\n")
            }
            
            let rootPath = installDir.appendingPathComponent(file)
            if fm.fileExists(atPath: rootPath.path) {
                try? fm.removeItem(at: rootPath)
                appendLog("[*] Removed EA modern patch file from root: \(file)\n")
            }
        }
    }

    func appendLog(_ text: String) {
        print(text, terminator: "")
        DispatchQueue.main.async {
            self.consoleLog += text
        }
    }

    func startDownload(username: String, password: String) {
        guard !state.isRunning else { return }

        consoleLog = ""
        lastUsername = username
        startedAt = Date()
        Analytics.logSteamDownloadStarted(needsSteamCMD: !isSteamCMDInstalled)

        if RosettaInstaller.isRequired(toRun: steamCMDBinary) {
            requestRosetta(username: username, password: password)
            return
        }

        if !isSteamCMDInstalled {
            installSteamCMD { [weak self] success in
                guard let self, success else { return }
                self.runSteamCMD(username: username, password: password)
            }
            return
        }

        runSteamCMD(username: username, password: password)
    }

    // MARK: - Rosetta

    func installRosetta() {
        guard !state.isRunning else { return }

        DispatchQueue.main.async { self.state = .installingRosetta }
        appendLog("[*] Installing Rosetta 2 — an administrator password is required...\n")

        RosettaInstaller.install { [weak self] result in
            DispatchQueue.main.async { self?.handleRosettaResult(result) }
        }
    }

    func declineRosetta() {
        pendingCredentials = nil
        fail("Rosetta 2 is required to run SteamCMD on Apple Silicon")
    }

    private func requestRosetta(username: String, password: String) {
        pendingCredentials = SteamCredentials(username: username, password: password)
        appendLog("[!] SteamCMD ships as an Intel-only binary and needs Rosetta 2.\n")
        DispatchQueue.main.async { self.showRosettaAlert = true }
    }

    private func handleRosettaResult(_ result: RosettaInstallResult) {
        switch result {
        case .installed:
            appendLog("[✓] Rosetta 2 installed\n\n")
            resumePendingDownload()

        case .cancelled:
            pendingCredentials = nil
            state = .idle
            appendLog("[*] Rosetta 2 installation cancelled\n")

        case .failed(let reason):
            pendingCredentials = nil
            fail("Rosetta 2 installation failed: \(reason)")
        }
    }

    private func resumePendingDownload() {
        guard let credentials = pendingCredentials else {
            state = .idle
            return
        }

        pendingCredentials = nil
        state = .idle
        startDownload(username: credentials.username, password: credentials.password)
    }

    func submitSteamGuard() {
        guard case .waitingSteamGuard = state, !steamGuardCode.isEmpty else { return }

        let code = steamGuardCode.trimmingCharacters(in: .whitespacesAndNewlines)
        appendLog("> Steam Guard code submitted\n")
        writeToProcess(code + "\n")
        steamGuardCode = ""
        DispatchQueue.main.async { self.state = .authenticating }
    }

    func cancel() {
        Analytics.logSteamDownloadCancelled(stage: state.analyticsStage)

        process?.terminate()
        process = nil
        inputPipe = nil
        DispatchQueue.main.async {
            self.state = .idle
            self.appendLog("\n--- CANCELLED ---\n")
        }
    }

    // MARK: - SteamCMD Installation

    private func installSteamCMD(completion: @escaping (Bool) -> Void) {
        DispatchQueue.main.async { self.state = .downloadingSteamCMD }
        appendLog("[*] Downloading SteamCMD for macOS...\n")

        let tarballURL = "https://steamcdn-a.akamaihd.net/client/installer/steamcmd_osx.tar.gz"

        DispatchQueue.global(qos: .userInitiated).async { [weak self] in
            guard let self else { return }

            do {
                try FileManager.default.createDirectory(at: self.steamCMDDir, withIntermediateDirectories: true)

                let tarPath = self.steamCMDDir.appendingPathComponent("steamcmd_osx.tar.gz")

                guard let url = URL(string: tarballURL),
                      let data = try? Data(contentsOf: url) else {
                    self.fail("Failed to download SteamCMD tarball")
                    completion(false)
                    return
                }

                try data.write(to: tarPath)
                self.appendLog("[*] Extracting SteamCMD...\n")

                let tar = Process()
                tar.executableURL = URL(fileURLWithPath: "/usr/bin/tar")
                tar.arguments = ["-xzf", tarPath.path, "-C", self.steamCMDDir.path]
                try tar.run()
                tar.waitUntilExit()

                guard tar.terminationStatus == 0 else {
                    self.fail("Failed to extract SteamCMD (exit \(tar.terminationStatus))")
                    completion(false)
                    return
                }

                try? FileManager.default.removeItem(at: tarPath)

                self.appendLog("[*] Removing quarantine attributes...\n")
                let xattr = Process()
                xattr.executableURL = URL(fileURLWithPath: "/usr/bin/xattr")
                xattr.arguments = ["-cr", self.steamCMDDir.path]
                try xattr.run()
                xattr.waitUntilExit()

                try FileManager.default.setAttributes(
                    [.posixPermissions: 0o755],
                    ofItemAtPath: self.steamCMDBinary.path
                )

                self.appendLog("[✓] SteamCMD installed successfully\n\n")
                completion(true)
            } catch {
                self.fail("Installation error: \(error.localizedDescription)")
                completion(false)
            }
        }
    }

    // MARK: - SteamCMD Execution

    private func runSteamCMD(username: String, password: String) {
        DispatchQueue.main.async { self.state = .authenticating }
        appendLog("[*] Launching SteamCMD...\n")

        do {
            try FileManager.default.createDirectory(at: assetsDir, withIntermediateDirectories: true)
        } catch {
            fail("Cannot create assets directory: \(error.localizedDescription)")
            return
        }

        let task = Process()
        task.executableURL = steamCMDBinary

        task.arguments = [
            "+@sSteamCmdForcePlatformType", "windows",
            "+force_install_dir", assetsDir.path,
            "+login", username, password,
            "+app_update", Self.appID, "validate",
            "+quit"
        ]

        task.currentDirectoryURL = steamCMDDir

        let outputPipe = Pipe()
        let errorPipe = Pipe()
        let input = Pipe()

        task.standardOutput = outputPipe
        task.standardError = errorPipe
        task.standardInput = input

        self.inputPipe = input
        self.process = task

        outputPipe.fileHandleForReading.readabilityHandler = { [weak self] handle in
            let data = handle.availableData
            guard !data.isEmpty, let text = String(data: data, encoding: .utf8) else { return }
            self?.processOutput(text)
        }

        errorPipe.fileHandleForReading.readabilityHandler = { [weak self] handle in
            let data = handle.availableData
            guard !data.isEmpty, let text = String(data: data, encoding: .utf8) else { return }
            self?.processOutput(text)
        }

        task.terminationHandler = { [weak self] proc in
            DispatchQueue.main.async {
                guard let self else { return }
                outputPipe.fileHandleForReading.readabilityHandler = nil
                errorPipe.fileHandleForReading.readabilityHandler = nil
                self.process = nil
                self.inputPipe = nil

                if proc.terminationStatus == 0 {
                    self.appendLog("\n[✓] Vanilla download completed successfully!\n")
                    self.reorganizeAssets()
                    self.cleanEAPatchFiles()
                    self.downloadCommunityPatch()
                } else if proc.terminationStatus == 42 {
                    self.appendLog("\n[*] SteamCMD updated itself. Restarting process...\n")
                    self.runSteamCMD(username: username, password: password)
                } else if case .failed = self.state {
                } else {
                    self.fail("SteamCMD exited with code \(proc.terminationStatus)")
                }
            }
        }

        do {
            try task.run()
        } catch {
            handleLaunchFailure(error, username: username, password: password)
        }
    }

    private func handleLaunchFailure(_ error: Error, username: String, password: String) {
        let needsRosetta = RosettaInstaller.isArchitectureFailure(error)
            || RosettaInstaller.isRequired(toRun: steamCMDBinary)

        guard needsRosetta else {
            fail("Failed to launch SteamCMD: \(error.localizedDescription)")
            return
        }

        pendingCredentials = SteamCredentials(username: username, password: password)
        fail("SteamCMD is an Intel-only binary and needs Rosetta 2")
        DispatchQueue.main.async { self.showRosettaAlert = true }
    }

    static let storeURL = "https://store.steampowered.com/app/2732960"

    private func processOutput(_ text: String) {
        appendLog(text)

        let lower = text.lowercased()

        if lower.contains("steam guard") || lower.contains("two-factor") || lower.contains("enter the current code") {
            DispatchQueue.main.async {
                guard self.state != .waitingSteamGuard else { return }
                self.state = .waitingSteamGuard
                Analytics.logSteamGuardPrompted()
            }
            return
        }

        if lower.contains("no subscription") || lower.contains("no license") {
            fail("Game not owned — purchase required")
            DispatchQueue.main.async { self.showPurchaseAlert = true }
            return
        }

        if lower.contains("update state") {
            let lines = text.components(separatedBy: "\n")
            for line in lines where line.lowercased().contains("update state") {
                let trimmed = line.trimmingCharacters(in: .whitespacesAndNewlines)
                DispatchQueue.main.async { self.state = .downloading(progress: trimmed) }
            }
            return
        }

        if lower.contains("validating") {
            DispatchQueue.main.async { self.state = .validating }
            return
        }

        if lower.contains("login failure") || lower.contains("invalid password") {
            fail("Authentication failed — check credentials")
            process?.terminate()
            return
        }
    }

    private func writeToProcess(_ text: String) {
        guard let data = text.data(using: .utf8) else { return }
        inputPipe?.fileHandleForWriting.write(data)
    }

    private func fail(_ message: String) {
        Analytics.logSteamDownloadFailed(reason: message)

        DispatchQueue.main.async {
            self.state = .failed(message)
            self.appendLog("\n[✗] \(message)\n")
        }
    }

    // MARK: - Community Patch

    private func downloadCommunityPatch() {
        DispatchQueue.main.async {
            self.state = .downloadingPatch(progress: 0.0)
            self.appendLog("\n[⭳] Downloading Community Patch & Maps...\n")
        }

        guard let patchURL = URL(string: AssetPatcher.patchURL) else {
            fail("Invalid patch URL.")
            return
        }

        let task = URLSession.shared.downloadTask(with: patchURL) { [weak self] localURL, response, error in
            guard let self = self else { return }

            if let error = error {
                self.fail("Failed to download patch: \(error.localizedDescription)")
                return
            }

            guard let localURL = localURL else {
                self.fail("Patch download failed: No file URL returned.")
                return
            }

            let tempZipURL = self.installDir.appendingPathComponent("temp_patch.zip")
            let fm = FileManager.default
            try? fm.removeItem(at: tempZipURL)
            do {
                try fm.moveItem(at: localURL, to: tempZipURL)
                DispatchQueue.main.async {
                    self.downloadObservation?.invalidate()
                    self.downloadObservation = nil
                    self.applyPatch(from: tempZipURL)
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

    private func applyPatch(from zipURL: URL) {
        state = .unpackingPatch
        appendLog("[*] Unpacking patch...\n")

        let extractDir = installDir.appendingPathComponent("temp_patch_extract")
        try? FileManager.default.removeItem(at: extractDir)
        try? FileManager.default.createDirectory(at: extractDir, withIntermediateDirectories: true)

        let process = Process()
        process.executableURL = URL(fileURLWithPath: "/usr/bin/unzip")
        process.arguments = ["-o", zipURL.path, "-d", extractDir.path]

        process.terminationHandler = { [weak self] proc in
            DispatchQueue.main.async {
                try? FileManager.default.removeItem(at: zipURL)
                
                guard let self = self else { return }

                guard proc.terminationStatus == 0 else {
                    try? FileManager.default.removeItem(at: extractDir)
                    self.fail("Failed to unpack patch (unzip exited with code \(proc.terminationStatus))")
                    return
                }

                do {
                    let applied = try AssetPatcher.applyExtractedPatch(
                        from: extractDir,
                        targets: self.patchTargets,
                        log: { [weak self] text in self?.appendLog(text) }
                    )
                    try? FileManager.default.removeItem(at: extractDir)

                    guard applied > 0 else {
                        self.fail("Patch archive carried no assets for the installed games.")
                        return
                    }

                    self.appendLog("[✓] Community Patch successfully applied!\n")
                    Analytics.logSteamDownloadFinished(seconds: self.elapsedSeconds)
                    self.state = .completed
                } catch {
                    try? FileManager.default.removeItem(at: extractDir)
                    self.fail("Failed to apply patch: \(error.localizedDescription)")
                }
            }
        }

        do {
            try process.run()
        } catch {
            fail("Failed to execute unzip: \(error.localizedDescription)")
        }
    }
}

// MARK: - Keychain

struct KeychainHelper {
    private static let service = "com.generals-online.launcher"

    static func save(account: String, password: String) {
        guard let data = password.data(using: .utf8) else { return }

        delete(account: account)

        let query: [String: Any] = [
            kSecClass as String: kSecClassGenericPassword,
            kSecAttrService as String: service,
            kSecAttrAccount as String: account,
            kSecValueData as String: data
        ]

        SecItemAdd(query as CFDictionary, nil)
    }

    static func load(account: String) -> String? {
        let query: [String: Any] = [
            kSecClass as String: kSecClassGenericPassword,
            kSecAttrService as String: service,
            kSecAttrAccount as String: account,
            kSecReturnData as String: true,
            kSecMatchLimit as String: kSecMatchLimitOne
        ]

        var result: AnyObject?
        let status = SecItemCopyMatching(query as CFDictionary, &result)

        guard status == errSecSuccess, let data = result as? Data else { return nil }
        return String(data: data, encoding: .utf8)
    }

    static func delete(account: String) {
        let query: [String: Any] = [
            kSecClass as String: kSecClassGenericPassword,
            kSecAttrService as String: service,
            kSecAttrAccount as String: account
        ]

        SecItemDelete(query as CFDictionary)
    }

    static func savedUsername() -> String? {
        let query: [String: Any] = [
            kSecClass as String: kSecClassGenericPassword,
            kSecAttrService as String: service,
            kSecReturnAttributes as String: true,
            kSecMatchLimit as String: kSecMatchLimitOne
        ]

        var result: AnyObject?
        let status = SecItemCopyMatching(query as CFDictionary, &result)

        guard status == errSecSuccess, let attrs = result as? [String: Any] else { return nil }
        return attrs[kSecAttrAccount as String] as? String
    }
}
