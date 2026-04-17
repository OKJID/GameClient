import Foundation
import AppKit
import Security

enum SteamCMDState: Equatable {
    case idle
    case downloadingSteamCMD
    case waitingForCredentials
    case authenticating
    case waitingSteamGuard
    case downloading(progress: String)
    case validating
    case completed
    case failed(String)

    var isRunning: Bool {
        switch self {
        case .downloadingSteamCMD, .authenticating, .downloading, .validating:
            return true
        default:
            return false
        }
    }

    var statusText: String {
        switch self {
        case .idle: return "READY"
        case .downloadingSteamCMD: return "INSTALLING STEAMCMD..."
        case .waitingForCredentials: return "AWAITING CREDENTIALS"
        case .authenticating: return "AUTHENTICATING..."
        case .waitingSteamGuard: return "STEAM GUARD CODE REQUIRED"
        case .downloading(let progress): return "DOWNLOADING ASSETS... \(progress)"
        case .validating: return "VALIDATING FILES..."
        case .completed: return "ASSETS READY"
        case .failed(let msg): return "ERROR: \(msg)"
        }
    }
}

class SteamCMDManager: ObservableObject {
    @Published var state: SteamCMDState = .idle
    @Published var consoleLog: String = ""
    @Published var steamGuardCode: String = ""
    @Published var showPurchaseAlert: Bool = false

    var lastUsername: String = ""

    static let appID = "2732960"

    private var process: Process?
    private var inputPipe: Pipe?

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

    var isSteamCMDInstalled: Bool {
        FileManager.default.fileExists(atPath: steamCMDBinary.path)
    }

    var areAssetsValid: Bool {
        let fm = FileManager.default
        let zhFiles = (try? fm.contentsOfDirectory(atPath: assetsDir.path)) ?? []
        let baseFiles = (try? fm.contentsOfDirectory(atPath: baseGameDir.path)) ?? []

        let hasZH = zhFiles.contains { $0.lowercased() == "inizh.big" }
        let hasBase = baseFiles.contains { $0.lowercased() == "ini.big" }

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

        if !isSteamCMDInstalled {
            installSteamCMD { [weak self] success in
                guard let self, success else { return }
                self.runSteamCMD(username: username, password: password)
            }
            return
        }

        runSteamCMD(username: username, password: password)
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
                    self.appendLog("\n[✓] Download completed successfully!\n")
                    self.reorganizeAssets()
                    self.state = .completed
                } else if case .failed = self.state {
                    // already set
                } else {
                    self.fail("SteamCMD exited with code \(proc.terminationStatus)")
                }
            }
        }

        do {
            try task.run()
        } catch {
            fail("Failed to launch SteamCMD: \(error.localizedDescription)")
        }
    }

    static let storeURL = "https://store.steampowered.com/app/2732960"

    private func processOutput(_ text: String) {
        appendLog(text)

        let lower = text.lowercased()

        if lower.contains("steam guard") || lower.contains("two-factor") || lower.contains("enter the current code") {
            DispatchQueue.main.async { self.state = .waitingSteamGuard }
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
        DispatchQueue.main.async {
            self.state = .failed(message)
            self.appendLog("\n[✗] \(message)\n")
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
