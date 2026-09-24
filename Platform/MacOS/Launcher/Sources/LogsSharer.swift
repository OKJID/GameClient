import AppKit
import Foundation

enum LogsSharer {
    static let telegramGroupURL = "https://t.me/GeneralsOnlineMacOS"
    static let telegramDeepLink = "tg://resolve?domain=GeneralsOnlineMacOS"

    static let crashReportNames = ["MacCrash.txt", "MacCrash.txt.bak", "MacCrash.txt.bak2", "MacCrash.txt.bak3"]

    private static func candidateLogs(in base: URL) -> [URL] {
        [
            base.appendingPathComponent("MacDebug.txt"),
            base.appendingPathComponent("GeneralsOnlineData/GeneralsOnline.log")
        ] + crashReportNames.map { base.appendingPathComponent($0) }
    }

    static func existingLogs(in base: URL) -> [URL] {
        candidateLogs(in: base).filter { FileManager.default.fileExists(atPath: $0.path) }
    }

    static func existingCrashReports(in base: URL) -> [URL] {
        crashReportNames
            .map { base.appendingPathComponent($0) }
            .filter { FileManager.default.fileExists(atPath: $0.path) }
    }

    static func share() {
        let files = existingLogs(in: GameProfile.current.userDataDirURL)
        if files.isEmpty {
            presentAlert(L10n.settings.shareLogsNone)
            return
        }

        DispatchQueue.global(qos: .userInitiated).async {
            let zipURL = makeZip(of: files, prefix: GameProfile.current.logsArchivePrefix)
            DispatchQueue.main.async {
                guard let zipURL = zipURL else {
                    presentAlert(L10n.settings.shareLogsFailed)
                    return
                }
                openTelegram()
                NSWorkspace.shared.activateFileViewerSelecting([zipURL])
            }
        }
    }

    private static func openTelegram() {
        if let deepLink = URL(string: telegramDeepLink), NSWorkspace.shared.open(deepLink) {
            return
        }

        if let webURL = URL(string: telegramGroupURL) {
            NSWorkspace.shared.open(webURL)
        }
    }

    static func makeZip(of files: [URL], prefix: String) -> URL? {
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy-MM-dd_HH-mm-ss"
        let stamp = formatter.string(from: Date())
        let zipURL = FileManager.default.temporaryDirectory
            .appendingPathComponent("\(prefix)\(stamp).zip")

        try? FileManager.default.removeItem(at: zipURL)

        let process = Process()
        process.executableURL = URL(fileURLWithPath: "/usr/bin/zip")
        process.arguments = ["-j", zipURL.path] + files.map { $0.path }

        do {
            try process.run()
            process.waitUntilExit()
        } catch {
            return nil
        }

        guard process.terminationStatus == 0,
              FileManager.default.fileExists(atPath: zipURL.path) else {
            return nil
        }
        return zipURL
    }

    private static func presentAlert(_ message: String) {
        let alert = NSAlert()
        alert.messageText = L10n.settings.shareLogs
        alert.informativeText = message
        alert.runModal()
    }
}
