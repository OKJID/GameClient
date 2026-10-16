import AppKit
import CryptoKit
import Foundation

enum LauncherUpdateState: Equatable {
    case idle
    case requestingAccess
    case downloading(progress: Double)
    case verifying
    case unpacking
    case installing
    case relaunching
    case accessDenied
    case failed(String)

    var isRunning: Bool {
        switch self {
        case .requestingAccess, .downloading, .verifying, .unpacking, .installing, .relaunching:
            return true
        default:
            return false
        }
    }

    var isFailure: Bool {
        switch self {
        case .accessDenied, .failed: return true
        default: return false
        }
    }

    var acceptsInstall: Bool {
        self == .idle || self == .accessDenied
    }

    var fraction: Double {
        switch self {
        case .idle, .requestingAccess, .accessDenied, .failed: return 0
        case .downloading(let progress): return progress * 0.8
        case .verifying: return 0.85
        case .unpacking: return 0.9
        case .installing: return 0.95
        case .relaunching: return 1
        }
    }

    var statusText: String {
        switch self {
        case .idle: return ""
        case .requestingAccess: return L10n.update.status.access
        case .downloading(let progress): return String(format: L10n.update.status.downloading, progress * 100)
        case .verifying: return L10n.update.status.verifying
        case .unpacking: return L10n.update.status.unpacking
        case .installing: return L10n.update.status.installing
        case .relaunching: return L10n.update.status.relaunching
        case .accessDenied:
            return L10n.update.accessDenied.replacingOccurrences(of: "%@", with: LauncherBundle.protectedLocation.displayName)
        case .failed(let message): return L10n.update.status.error.replacingOccurrences(of: "%@", with: message)
        }
    }
}

enum UpdateSignature {
    private static let publicKeyInfoKey = "GOUpdatePublicKey"

    static var publicKey: Curve25519.Signing.PublicKey? {
        guard let encoded = Bundle.main.object(forInfoDictionaryKey: publicKeyInfoKey) as? String,
              let raw = Data(base64Encoded: encoded) else {
            return nil
        }

        return try? Curve25519.Signing.PublicKey(rawRepresentation: raw)
    }

    static func message(version: String, build: Int, sha256: String) -> Data {
        Data("\(version)\n\(build)\n\(sha256.lowercased())".utf8)
    }

    static func isAuthentic(_ update: AppUpdate) -> Bool {
        guard let publicKey,
              let sha256 = update.sha256,
              let encoded = update.signature,
              let signature = Data(base64Encoded: encoded) else {
            return false
        }

        return publicKey.isValidSignature(signature, for: message(version: update.version, build: update.build, sha256: sha256))
    }
}

class LauncherUpdater: ObservableObject {
    @Published private(set) var state: LauncherUpdateState = .idle

    private var downloadObservation: NSKeyValueObservation?
    private var startedAt: Date?

    private static var stagingURL: URL? {
        ApiCache.folderURL?.appendingPathComponent("LauncherUpdate", isDirectory: true)
    }

    private var elapsedSeconds: Double {
        guard let startedAt else { return 0 }
        return Date().timeIntervalSince(startedAt)
    }

    // MARK: - Public API

    func canInstall(_ update: AppUpdate) -> Bool {
        guard UpdateSignature.publicKey != nil else { return false }
        guard update.sha256 != nil, update.signature != nil else { return false }
        return LauncherBundle.isReplaceable
    }

    func install(_ update: AppUpdate) {
        guard state.acceptsInstall else { return }
        begin(update)
    }

    func requestAccessAgain(_ update: AppUpdate) {
        guard state == .accessDenied else { return }

        state = .requestingAccess
        LauncherBundle.resetPrivacyDecision { [weak self] status in
            print("[update] tccutil reset \(LauncherBundle.protectedLocation.privacyService ?? "-") → exit \(status)")

            DispatchQueue.main.async {
                self?.begin(update)
            }
        }
    }

    func clearFailure() {
        guard state.isFailure else { return }
        state = .idle
    }

    private func begin(_ update: AppUpdate) {
        startedAt = Date()
        Analytics.logUpdateInstallStarted(version: update.version)
        print("[update] Installing v\(update.version) (\(update.build)) from \(update.downloadURL)")

        guard UpdateSignature.isAuthentic(update) else {
            fail(update, stage: "signature", "The update feed signature is invalid.")
            return
        }

        guard let source = URL(string: update.downloadURL) else {
            fail(update, stage: "download", "Invalid download URL.")
            return
        }

        guard let staging = Self.stagingURL, (try? Self.resetStaging(staging)) != nil else {
            fail(update, stage: "staging", "Application Support is not writable.")
            return
        }

        download(update, from: source, into: staging)
    }

    // MARK: - Download

    private func download(_ update: AppUpdate, from source: URL, into staging: URL) {
        state = .downloading(progress: 0)
        let archive = staging.appendingPathComponent("update.zip")

        let task = URLSession.shared.downloadTask(with: source) { [weak self] localURL, response, error in
            guard let self else { return }

            let statusCode = (response as? HTTPURLResponse)?.statusCode ?? 0
            guard error == nil, statusCode == 200, let localURL else {
                self.fail(update, stage: "download", error?.localizedDescription ?? "HTTP \(statusCode)")
                return
            }

            do {
                try FileManager.default.moveItem(at: localURL, to: archive)
            } catch {
                self.fail(update, stage: "download", error.localizedDescription)
                return
            }

            self.verify(update, archive: archive, staging: staging)
        }

        downloadObservation = task.progress.observe(\.fractionCompleted) { [weak self] progress, _ in
            let fraction = progress.fractionCompleted
            DispatchQueue.main.async {
                guard case .downloading = self?.state else { return }
                self?.state = .downloading(progress: fraction)
            }
        }

        task.resume()
    }

    // MARK: - Verify

    private func verify(_ update: AppUpdate, archive: URL, staging: URL) {
        DispatchQueue.main.async {
            self.stopObservingDownload()
            self.state = .verifying
        }

        DispatchQueue.global(qos: .userInitiated).async {
            guard let digest = try? Self.sha256Hex(of: archive) else {
                self.fail(update, stage: "checksum", "The downloaded archive is unreadable.")
                return
            }

            guard digest == update.sha256?.lowercased() else {
                self.fail(update, stage: "checksum", "The downloaded archive does not match the published checksum.")
                return
            }

            DispatchQueue.main.async {
                self.unpack(update, archive: archive, staging: staging)
            }
        }
    }

    private static func sha256Hex(of file: URL) throws -> String {
        let handle = try FileHandle(forReadingFrom: file)
        defer { try? handle.close() }

        var hasher = SHA256()
        while let chunk = try handle.read(upToCount: 1 << 20), !chunk.isEmpty {
            hasher.update(data: chunk)
        }

        return hasher.finalize().map { String(format: "%02x", $0) }.joined()
    }

    // MARK: - Unpack

    private func unpack(_ update: AppUpdate, archive: URL, staging: URL) {
        state = .unpacking
        let extracted = staging.appendingPathComponent("extracted", isDirectory: true)

        Self.runTool("/usr/bin/ditto", ["-x", "-k", archive.path, extracted.path]) { [weak self] status in
            guard let self else { return }
            try? FileManager.default.removeItem(at: archive)

            guard status == 0 else {
                self.fail(update, stage: "unpack", "ditto exited with code \(status).")
                return
            }

            self.validate(update, extracted: extracted, staging: staging)
        }
    }

    private func validate(_ update: AppUpdate, extracted: URL, staging: URL) {
        guard let freshBundle = Self.findBundle(in: extracted) else {
            fail(update, stage: "validate", "The archive carries no application.")
            return
        }

        guard Self.bundle(freshBundle, matches: update) else {
            fail(update, stage: "validate", "The archive carries a different launcher version.")
            return
        }

        Self.runTool("/usr/bin/codesign", ["--verify", "--deep", "--strict", freshBundle.path]) { [weak self] status in
            guard let self else { return }
            guard status == 0 else {
                self.fail(update, stage: "codesign", "codesign exited with code \(status).")
                return
            }

            Self.runTool("/usr/bin/xattr", ["-dr", "com.apple.quarantine", freshBundle.path]) { _ in
                DispatchQueue.main.async {
                    self.replace(with: freshBundle, update: update, staging: staging)
                }
            }
        }
    }

    private static func findBundle(in directory: URL) -> URL? {
        let items = (try? FileManager.default.contentsOfDirectory(at: directory, includingPropertiesForKeys: nil)) ?? []
        return items.first { $0.pathExtension == "app" }
    }

    private static func bundle(_ bundleURL: URL, matches update: AppUpdate) -> Bool {
        let plistURL = bundleURL.appendingPathComponent("Contents/Info.plist")
        guard let info = NSDictionary(contentsOf: plistURL) else { return false }

        return info["GOLauncherVersion"] as? String == update.version
            && info["GOLauncherBuild"] as? String == String(update.build)
    }

    // MARK: - Install

    private func replace(with freshBundle: URL, update: AppUpdate, staging: URL) {
        guard !LauncherBundle.hasRunningSiblings else {
            fail(update, stage: "install", L10n.update.gameRunning)
            return
        }

        state = .installing

        DispatchQueue.global(qos: .userInitiated).async {
            do {
                try LauncherBundle.replaceContents(with: freshBundle)
            } catch {
                self.failInstall(update, error)
                return
            }

            DispatchQueue.main.async {
                self.relaunch(update, staging: staging)
            }
        }
    }

    private func failInstall(_ update: AppUpdate, _ error: Error) {
        print("[update] replace error: \(error as NSError)")

        guard !LauncherBundle.isPrivacyDenial(error) else {
            denyAccess(update)
            return
        }

        fail(update, stage: "install", error.localizedDescription)
    }

    // MARK: - Relaunch

    private func relaunch(_ update: AppUpdate, staging: URL) {
        state = .relaunching
        print("[update] ✓ v\(update.version) installed into \(LauncherBundle.url.path), relaunching")

        do {
            try LauncherBundle.scheduleRelaunch(log: staging.appendingPathComponent("relaunch.log"))
        } catch {
            fail(update, stage: "relaunch", "The update is installed, reopen the launcher manually.")
            return
        }

        Analytics.logUpdateInstallReady(version: update.version, seconds: elapsedSeconds) {
            NSApplication.shared.terminate(nil)
        }
    }

    // MARK: - Helpers

    private static func resetStaging(_ url: URL) throws {
        let fm = FileManager.default
        if fm.fileExists(atPath: url.path) {
            try fm.removeItem(at: url)
        }

        try fm.createDirectory(at: url, withIntermediateDirectories: true)
    }

    private static func runTool(_ path: String, _ arguments: [String], completion: @escaping (Int32) -> Void) {
        let process = Process()
        process.executableURL = URL(fileURLWithPath: path)
        process.arguments = arguments
        process.terminationHandler = { completion($0.terminationStatus) }

        do {
            try process.run()
        } catch {
            completion(-1)
        }
    }

    private func stopObservingDownload() {
        downloadObservation?.invalidate()
        downloadObservation = nil
    }

    private func denyAccess(_ update: AppUpdate) {
        print("[update] ✗ access: macOS denied write access to \(LauncherBundle.url.path)")
        Analytics.logUpdateInstallFailed(version: update.version, stage: "access", reason: "privacy_denied")

        DispatchQueue.main.async {
            self.stopObservingDownload()
            self.state = .accessDenied
        }
    }

    private func fail(_ update: AppUpdate, stage: String, _ message: String) {
        print("[update] ✗ \(stage): \(message)")
        Analytics.logUpdateInstallFailed(version: update.version, stage: stage, reason: message)

        DispatchQueue.main.async {
            self.stopObservingDownload()
            self.state = .failed(message)
        }
    }
}
