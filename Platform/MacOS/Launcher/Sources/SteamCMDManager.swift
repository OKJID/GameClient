import Foundation
import AppKit

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

    static let appID = "2732960"

    private var process: Process?
    private var inputPipe: Pipe?

    var supportDir: URL {
        let base = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask).first!
        return base.appendingPathComponent("Generals Online")
    }

    var steamCMDDir: URL { supportDir.appendingPathComponent("steamcmd") }
    var steamCMDBinary: URL { steamCMDDir.appendingPathComponent("steamcmd") }
    var assetsDir: URL {
        let docs = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first!
        return docs.appendingPathComponent("Generals Online").appendingPathComponent("Assets")
    }

    var isSteamCMDInstalled: Bool {
        FileManager.default.fileExists(atPath: steamCMDBinary.path)
    }

    var areAssetsValid: Bool {
        let fm = FileManager.default
        guard let items = try? fm.contentsOfDirectory(at: assetsDir, includingPropertiesForKeys: [.isDirectoryKey], options: .skipsHiddenFiles) else {
            return false
        }

        var hasZH = false
        var hasBase = false

        for itemURL in items {
            guard let isDir = try? itemURL.resourceValues(forKeys: [.isDirectoryKey]).isDirectory, isDir else { continue }
            guard let subItems = try? fm.contentsOfDirectory(atPath: itemURL.path) else { continue }

            let containsZH = subItems.contains { $0.lowercased() == "inizh.big" }
            let containsBase = subItems.contains { $0.lowercased() == "ini.big" }

            if containsZH { hasZH = true }
            else if containsBase { hasBase = true }

            if hasZH && hasBase { return true }
        }

        return false
    }

    func appendLog(_ text: String) {
        DispatchQueue.main.async {
            self.consoleLog += text
        }
    }

    func startDownload(username: String, password: String) {
        guard !state.isRunning else { return }

        consoleLog = ""

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
            "+login", username, password,
            "+force_install_dir", assetsDir.path,
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

    private func processOutput(_ text: String) {
        appendLog(text)

        let lower = text.lowercased()

        if lower.contains("steam guard") || lower.contains("two-factor") || lower.contains("enter the current code") {
            DispatchQueue.main.async { self.state = .waitingSteamGuard }
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
