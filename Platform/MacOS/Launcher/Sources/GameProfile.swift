import SwiftUI

struct LauncherTheme {
    let accent: Color
    let accentSoft: Color
    let panel: Color
    let panelBorder: Color

    static let zeroHour = LauncherTheme(
        accent: Color(red: 0.10, green: 0.50, blue: 1.00),
        accentSoft: Color(red: 0.45, green: 0.72, blue: 1.00),
        panel: Color(red: 0.06, green: 0.09, blue: 0.14),
        panelBorder: Color(red: 0.16, green: 0.34, blue: 0.58)
    )

    static let generals = LauncherTheme(
        accent: Color(red: 0.85, green: 0.58, blue: 0.22),
        accentSoft: Color(red: 0.93, green: 0.78, blue: 0.52),
        panel: Color(red: 0.12, green: 0.09, blue: 0.06),
        panelBorder: Color(red: 0.52, green: 0.36, blue: 0.18)
    )
}

enum SettingKey: String, CaseIterable, Identifiable {
    case gameLanguage
    case windowedEdgeScroll
    case showHotkeyLabels
    case cameraMaxHeight
    case cameraMinHeight
    case cameraSpeed
    case limitFramerate
    case fpsLimit
    case statsOverlay
    case altEndpoint
    case verboseLogging

    var id: String { rawValue }
}

enum GameID: String, CaseIterable, Identifiable {
    case zeroHour = "z_generals"
    case generals = "g_generals"

    var id: String { rawValue }
}

struct GameProfile: Identifiable {
    let id: GameID
    let displayName: String
    let shortName: String
    let logsArchivePrefix: String
    let theme: LauncherTheme
    let userDataDirName: String
    let requiredArchive: String
    let forbiddenArchive: String?
    let bundleName: String
    let executableName: String
    let patchAssetsDirName: String
    let patchMarkers: [String]
    let localeLanguages: [String]
    let localeRequiredFiles: [String]
    let localeDirName: String
    let localeFileName: String
    let onlineSettingsRelativePath: String?
    let supportedSettings: Set<SettingKey>

    var localeMarkers: [String] {
        localeLanguages.flatMap { language in
            localeRequiredFiles.map { "\(localeDirName)/\(language)/\($0)" }
        }
    }

    var optionsFileURL: URL {
        userDataDirURL.appendingPathComponent("Options.ini")
    }

    var onlineSettingsFileURL: URL? {
        guard let relativePath = onlineSettingsRelativePath else {
            return nil
        }

        return userDataDirURL.appendingPathComponent(relativePath)
    }

    func supports(_ key: SettingKey) -> Bool {
        supportedSettings.contains(key)
    }

    func supportsAny(_ keys: [SettingKey]) -> Bool {
        keys.contains { supportedSettings.contains($0) }
    }

    var userDataDirURL: URL {
        FileManager.default.homeDirectoryForCurrentUser.appendingPathComponent(userDataDirName)
    }

    func matchesInstall(at directory: URL) -> Bool {
        let fm = FileManager.default
        guard let entries = try? fm.contentsOfDirectory(atPath: directory.path) else {
            return false
        }

        let archives = Set(entries.map { $0.lowercased() })
        guard archives.contains(requiredArchive.lowercased()) else {
            return false
        }

        guard let forbidden = forbiddenArchive else {
            return true
        }

        return !archives.contains(forbidden.lowercased())
    }

    func isPatchInstalled(at directory: URL) -> Bool {
        let fm = FileManager.default
        return (patchMarkers + localeMarkers).allSatisfy { marker in
            fm.fileExists(atPath: directory.appendingPathComponent(marker).path)
        }
    }

    func installedLanguages(at directory: URL) -> [String] {
        let fm = FileManager.default
        let localeRoot = directory.appendingPathComponent(localeDirName)
        guard let entries = try? fm.contentsOfDirectory(
            at: localeRoot,
            includingPropertiesForKeys: [.isDirectoryKey],
            options: .skipsHiddenFiles
        ) else {
            return GameProfile.fallbackLanguages
        }

        let found = entries.compactMap { entry -> String? in
            guard fm.fileExists(atPath: entry.appendingPathComponent(localeFileName).path) else {
                return nil
            }

            return entry.lastPathComponent.lowercased()
        }

        guard !found.isEmpty else {
            return GameProfile.fallbackLanguages
        }

        return Set(found).sorted(by: GameProfile.languagePrecedes)
    }
}

extension GameProfile {
    static let patchLanguages = [
        "English", "German", "French", "Spanish", "Italian",
        "Korean", "Chinese", "Polish", "Brazilian",
        "Russian", "Ukrainian"
    ]

    static let localeFilesRequiredToBoot = [
        "generals.csf",
        "Language.ini",
        "HeaderTemplate.ini",
        "CommandMap.ini"
    ]

    static let fallbackLanguages = [SettingsDefaults.gameLanguage]

    private static let languageOrder = [
        "english", "german", "french", "spanish", "italian",
        "korean", "chinese", "polish", "brazilian",
        "russian", "ukrainian"
    ]

    private static func languagePrecedes(_ lhs: String, _ rhs: String) -> Bool {
        let lhsRank = languageOrder.firstIndex(of: lhs) ?? languageOrder.count
        let rhsRank = languageOrder.firstIndex(of: rhs) ?? languageOrder.count
        guard lhsRank == rhsRank else {
            return lhsRank < rhsRank
        }

        return lhs < rhs
    }

    static let zeroHour = GameProfile(
        id: .zeroHour,
        displayName: "Generals Zero Hour",
        shortName: "ZERO HOUR",
        logsArchivePrefix: "GeneralsZHLogs",
        theme: .zeroHour,
        userDataDirName: "Command and Conquer Generals Zero Hour Data",
        requiredArchive: "INIZH.big",
        forbiddenArchive: nil,
        bundleName: "GeneralsOnlineZH.app",
        executableName: "GeneralsOnlineZH",
        patchAssetsDirName: "Assets",
        patchMarkers: [
            "310_ExpandedLANLobbyMenu.big",
            "340_ControlBarPro1080ZH.big",
            "340_ControlBarProHideIpZH.big",
            "340_ControlBarProHideMailZH.big",
            "340_ControlBarProZH.big",
            "400_ControlBarHDBaseZH.big",
            "400_ControlBarHDEnglishZH.big",
            "990_DecalsZH.big"
        ],
        localeLanguages: GameProfile.patchLanguages,
        localeRequiredFiles: GameProfile.localeFilesRequiredToBoot,
        localeDirName: "Data",
        localeFileName: "generals.csf",
        onlineSettingsRelativePath: "GeneralsOnlineData/settings.json",
        supportedSettings: [
            .gameLanguage,
            .windowedEdgeScroll,
            .showHotkeyLabels,
            .cameraMaxHeight,
            .cameraMinHeight,
            .cameraSpeed,
            .limitFramerate,
            .fpsLimit,
            .statsOverlay,
            .altEndpoint,
            .verboseLogging
        ]
    )

    static let generals = GameProfile(
        id: .generals,
        displayName: "Generals",
        shortName: "GENERALS",
        logsArchivePrefix: "GeneralsLogs",
        theme: .generals,
        userDataDirName: "Command and Conquer Generals Data",
        requiredArchive: "INI.big",
        forbiddenArchive: "INIZH.big",
        bundleName: "GeneralsVanilla.app",
        executableName: "GeneralsVanilla",
        patchAssetsDirName: "AssetsGenerals",
        patchMarkers: [
            "GenTool/fullviewport.dat",
            "Window/ControlBar.wnd",
            "Data/INI/ControlBarScheme.ini"
        ],
        localeLanguages: GameProfile.patchLanguages,
        localeRequiredFiles: GameProfile.localeFilesRequiredToBoot,
        localeDirName: "Data",
        localeFileName: "generals.csf",
        onlineSettingsRelativePath: nil,
        supportedSettings: [
            .gameLanguage,
            .windowedEdgeScroll,
            .showHotkeyLabels,
            .verboseLogging
        ]
    )

    static let all: [GameProfile] = [.zeroHour, .generals]

    private static let selectionKey = "SelectedGameID"

    static var selectedID: GameID {
        get {
            guard let raw = UserDefaults.standard.string(forKey: selectionKey),
                  let id = GameID(rawValue: raw) else {
                return .zeroHour
            }

            return id
        }
        set {
            UserDefaults.standard.set(newValue.rawValue, forKey: selectionKey)
        }
    }

    static var current: GameProfile {
        profile(for: selectedID)
    }

    static func profile(for id: GameID) -> GameProfile {
        all.first { $0.id == id } ?? .zeroHour
    }

    static func installDirectory(for id: GameID, under root: URL) -> URL? {
        let profile = profile(for: id)
        let fm = FileManager.default
        guard let entries = try? fm.contentsOfDirectory(
            at: root,
            includingPropertiesForKeys: [.isDirectoryKey],
            options: .skipsHiddenFiles
        ) else {
            return nil
        }

        return entries.first { entry in
            guard let isDir = try? entry.resourceValues(forKeys: [.isDirectoryKey]).isDirectory, isDir else {
                return false
            }

            return profile.matchesInstall(at: entry)
        }
    }
}
