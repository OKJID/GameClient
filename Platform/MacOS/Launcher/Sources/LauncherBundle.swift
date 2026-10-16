import AppKit
import Foundation

enum ProtectedLocation {
    case downloads
    case desktop
    case documents
    case networkVolume
    case removableVolume
    case unknown

    var privacyService: String? {
        switch self {
        case .downloads: return "SystemPolicyDownloadsFolder"
        case .desktop: return "SystemPolicyDesktopFolder"
        case .documents: return "SystemPolicyDocumentsFolder"
        case .networkVolume: return "SystemPolicyNetworkVolumes"
        case .removableVolume: return "SystemPolicyRemovableVolumes"
        case .unknown: return nil
        }
    }

    var displayName: String {
        switch self {
        case .downloads: return L10n.update.location.downloads
        case .desktop: return L10n.update.location.desktop
        case .documents: return L10n.update.location.documents
        case .networkVolume: return L10n.update.location.network
        case .removableVolume: return L10n.update.location.removable
        case .unknown: return L10n.update.location.unknown
        }
    }
}

enum LauncherBundle {
    static let privacySettingsURL = URL(string: "x-apple.systempreferences:com.apple.preference.security?Privacy_FilesAndFolders")!

    static var url: URL {
        Bundle.main.bundleURL.standardizedFileURL
    }

    private static var contentsURL: URL {
        url.appendingPathComponent("Contents", isDirectory: true)
    }

    private static var previousContentsURL: URL {
        url.appendingPathComponent("Contents.previous", isDirectory: true)
    }

    private static var executablesPath: String {
        contentsURL.appendingPathComponent("MacOS", isDirectory: true).path + "/"
    }

    static var isReplaceable: Bool {
        guard url.pathExtension == "app" else { return false }
        guard !url.path.contains("/AppTranslocation/") else { return false }

        let fm = FileManager.default
        return fm.isWritableFile(atPath: url.path) && fm.isWritableFile(atPath: contentsURL.path)
    }

    static var hasRunningSiblings: Bool {
        let ownPID = ProcessInfo.processInfo.processIdentifier

        return NSWorkspace.shared.runningApplications.contains { app in
            guard app.processIdentifier != ownPID else { return false }
            guard let path = app.executableURL?.standardizedFileURL.path else { return false }
            return path.hasPrefix(executablesPath)
        }
    }

    static func isPrivacyDenial(_ error: Error) -> Bool {
        let underlying = (error as NSError).userInfo[NSUnderlyingErrorKey] as? NSError
        let posix = underlying ?? (error as NSError)
        return posix.domain == NSPOSIXErrorDomain && posix.code == Int(EPERM)
    }

    // MARK: - Privacy

    private static let protectedFolders: [(FileManager.SearchPathDirectory, ProtectedLocation)] = [
        (.downloadsDirectory, .downloads),
        (.desktopDirectory, .desktop),
        (.documentDirectory, .documents)
    ]

    static var protectedLocation: ProtectedLocation {
        let bundlePath = url.resolvingSymlinksInPath().path

        for (directory, location) in protectedFolders {
            guard let folder = FileManager.default.urls(for: directory, in: .userDomainMask).first else { continue }
            if bundlePath.hasPrefix(folder.resolvingSymlinksInPath().path + "/") {
                return location
            }
        }

        let volume = try? url.resourceValues(forKeys: [.volumeIsLocalKey, .volumeIsRemovableKey, .volumeIsEjectableKey])
        if volume?.volumeIsLocal == false {
            return .networkVolume
        }

        if volume?.volumeIsRemovable == true || volume?.volumeIsEjectable == true {
            return .removableVolume
        }

        return .unknown
    }

    static var canRequestAccessAgain: Bool {
        protectedLocation.privacyService != nil && Bundle.main.bundleIdentifier != nil
    }

    static func resetPrivacyDecision(completion: @escaping (Int32) -> Void) {
        guard let service = protectedLocation.privacyService, let bundleID = Bundle.main.bundleIdentifier else {
            completion(-1)
            return
        }

        let process = Process()
        process.executableURL = URL(fileURLWithPath: "/usr/bin/tccutil")
        process.arguments = ["reset", service, bundleID]
        process.standardOutput = FileHandle.nullDevice
        process.standardError = FileHandle.nullDevice
        process.terminationHandler = { completion($0.terminationStatus) }

        do {
            try process.run()
        } catch {
            completion(-1)
        }
    }

    // MARK: - Replace

    static func replaceContents(with freshBundle: URL) throws {
        let fm = FileManager.default
        let freshContents = freshBundle.appendingPathComponent("Contents", isDirectory: true)

        if fm.fileExists(atPath: previousContentsURL.path) {
            try fm.removeItem(at: previousContentsURL)
        }

        try fm.moveItem(at: contentsURL, to: previousContentsURL)

        do {
            try fm.moveItem(at: freshContents, to: contentsURL)
        } catch {
            print("[update] replace: moving the new Contents in failed, rolling back")
            try? fm.removeItem(at: contentsURL)
            try? fm.moveItem(at: previousContentsURL, to: contentsURL)
            throw error
        }

        try? fm.removeItem(at: previousContentsURL)
        try? fm.setAttributes([.modificationDate: Date()], ofItemAtPath: url.path)
    }

    // MARK: - Relaunch

    private static let relaunchScript = """
    launcher_pid="$1"
    bundle="$2"

    while kill -0 "$launcher_pid" 2>/dev/null; do
        sleep 0.2
    done

    echo "opening $bundle"
    open "$bundle"
    """

    static func scheduleRelaunch(log logURL: URL) throws {
        FileManager.default.createFile(atPath: logURL.path, contents: nil)
        let logHandle = try FileHandle(forWritingTo: logURL)

        let process = Process()
        process.executableURL = URL(fileURLWithPath: "/bin/sh")
        process.arguments = [
            "-c", relaunchScript, "launcher-relaunch",
            String(ProcessInfo.processInfo.processIdentifier),
            url.path
        ]
        process.standardOutput = logHandle
        process.standardError = logHandle

        try process.run()
    }
}
