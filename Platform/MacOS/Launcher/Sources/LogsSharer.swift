import AppKit
import Foundation

enum LogsSharer {
    static let telegramGroupURL = "https://t.me/GeneralsOnlineMacOS"
    static let telegramDeepLink = "tg://resolve?domain=GeneralsOnlineMacOS"

    private static var candidateLogs: [URL] {
        let base = GameProfile.current.userDataDirURL
        return [
            base.appendingPathComponent("MacDebug.txt"),
            base.appendingPathComponent("GeneralsOnlineData/GeneralsOnline.log")
        ]
    }

    private static var existingLogs: [URL] {
        candidateLogs.filter { FileManager.default.fileExists(atPath: $0.path) }
    }

    static func share() {
        let files = existingLogs
        if files.isEmpty {
            presentAlert(L10n.settings.shareLogsNone)
            return
        }

        DispatchQueue.global(qos: .userInitiated).async {
            let zipURL = makeZip(of: files)
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

    private static func makeZip(of files: [URL]) -> URL? {
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy-MM-dd_HH-mm-ss"
        let stamp = formatter.string(from: Date())
        let zipURL = FileManager.default.temporaryDirectory
            .appendingPathComponent("\(GameProfile.current.logsArchivePrefix)\(stamp).zip")

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
