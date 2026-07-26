import SwiftUI

struct LauncherTheme {
    let accent: Color
    let accentSoft: Color
    let panel: Color
    let panelBorder: Color
    let backgroundImageName: String

    static let zeroHour = LauncherTheme(
        accent: Color(red: 0.10, green: 0.50, blue: 1.00),
        accentSoft: Color(red: 0.45, green: 0.72, blue: 1.00),
        panel: Color(red: 0.06, green: 0.09, blue: 0.14),
        panelBorder: Color(red: 0.16, green: 0.34, blue: 0.58),
        backgroundImageName: "background"
    )

    static let generals = LauncherTheme(
        accent: Color(red: 0.90, green: 0.32, blue: 0.10),
        accentSoft: Color(red: 1.00, green: 0.60, blue: 0.32),
        panel: Color(red: 0.14, green: 0.06, blue: 0.03),
        panelBorder: Color(red: 0.64, green: 0.26, blue: 0.09),
        backgroundImageName: "background"
    )

    static let contra = LauncherTheme(
        accent: Color(red: 1.00, green: 0.78, blue: 0.18),
        accentSoft: Color(red: 1.00, green: 0.90, blue: 0.55),
        panel: Color(red: 0.14, green: 0.11, blue: 0.03),
        panelBorder: Color(red: 0.72, green: 0.55, blue: 0.14),
        backgroundImageName: "background_mod"
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
    case contra007 = "m_contra007"
    case contra008 = "m_contra008"
    case contra009 = "m_contra009"
    case contraX = "m_contrax"

    var id: String { rawValue }
}

// A mod runs on top of a base game: same executable and user data, extra content
// unpacked into $INSTALL/Mods/<dirName>/. Multi-part packages list several urls.
struct ModSpec {
    let baseGameID: GameID
    let dirName: String
    let version: String
    let downloadURLs: [URL]
    let downloadSizeMB: Int
    let diskSizeMB: Int
    // Every file the package ships. Multi-part packages list files from all parts,
    // so a missing part reads as a damaged install.
    let markers: [String]

    static let configFileName = "config.json"
    static let modsDirName = "Mods"

    var partCount: Int { downloadURLs.count }
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
    let mod: ModSpec?

    var isMod: Bool { mod != nil }

    var baseProfile: GameProfile {
        guard let mod else { return self }
        return GameProfile.profile(for: mod.baseGameID)
    }

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

    func modDirectory(installRoot: URL) -> URL? {
        guard let mod else { return nil }
        return installRoot
            .appendingPathComponent(ModSpec.modsDirName)
            .appendingPathComponent(mod.dirName)
    }

    func missingModFiles(installRoot: URL) -> [String] {
        guard let mod, let directory = modDirectory(installRoot: installRoot) else { return [] }

        let fm = FileManager.default
        return mod.markers.filter { marker in
            !fm.fileExists(atPath: directory.appendingPathComponent(marker).path)
        }
    }

    func isModInstalled(installRoot: URL) -> Bool {
        guard isMod else { return false }
        return missingModFiles(installRoot: installRoot).isEmpty
    }

    func isModDamaged(installRoot: URL) -> Bool {
        guard let directory = modDirectory(installRoot: installRoot),
              FileManager.default.fileExists(atPath: directory.path) else {
            return false
        }

        return !missingModFiles(installRoot: installRoot).isEmpty
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
        ],
        mod: nil
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
        ],
        mod: nil
    )

    static let contra007 = zeroHourMod(
        id: .contra007,
        displayName: "Contra 007",
        shortName: "CONTRA 007",
        dirName: "Contra007",
        version: "0.07.1",
        assetNames: ["GO_Mac_Mod_Contra007.zip"],
        downloadSizeMB: 153,
        diskSizeMB: 286,
        markers: [
            "00_Contra007.big",
            "01_Contra007-en.big",
            "02_Contra006Music.big",
            "03_Contra-007-Fix.big",
            "00000000.016",
            "00000000.256",
            "ariag.ttf",
            "Install_Final.bmp",
            "Data/INI/GameData.ini",
            "Data/Scripts/SkirmishScripts.scb",
            "Maps/MapCache.ini"
        ]
    )

    static let contra008 = zeroHourMod(
        id: .contra008,
        displayName: "Contra 008",
        shortName: "CONTRA 008",
        dirName: "Contra008",
        version: "8.0.0",
        assetNames: ["GO_Mac_Mod_Contra008.zip"],
        downloadSizeMB: 595,
        diskSizeMB: 940,
        markers: [
            "00_Contra008.big",
            "01_Contra008EN.big",
            "02_Contra008VLoc.big",
            "03_Contra008MNew.big",
            "Install_Final.bmp"
        ]
    )

    static let contra009 = zeroHourMod(
        id: .contra009,
        displayName: "Contra 009",
        shortName: "CONTRA 009",
        dirName: "Contra009",
        version: "9.0.0-hf4",
        assetNames: ["GO_Mac_Mod_Contra009.zip"],
        downloadSizeMB: 1266,
        diskSizeMB: 1900,
        markers: [
            "00_Contra009Final.big",
            "01_Contra009Final_EN.big",
            "02_Contra009Final_EngVO.big",
            "03_Contra009Final_NewMusic.big",
            "10_Patch1.big",
            "11_Patch1_EN.big",
            "12_Patch1_EngVO.big",
            "20_Patch2.big",
            "21_Patch2_EN.big",
            "22_Patch2_EngVO.big",
            "23_Patch2_GameData.big",
            "30_Patch3.big",
            "31_Patch3_EN_Leikeze.big",
            "32_Patch3_EngVO.big",
            "33_Patch3_GameData.big",
            "40_Hotfix4.big",
            "41_Hotfix4_EN_Leikeze.big",
            "GenArial.ttf",
            "Install_Final.bmp"
        ]
    )

    static let contraX = zeroHourMod(
        id: .contraX,
        displayName: "Contra X",
        shortName: "CONTRA X",
        dirName: "ContraX",
        version: "x-beta2-p1",
        assetNames: [
            "GO_Mac_Mod_ContraX.1.zip",
            "GO_Mac_Mod_ContraX.2.zip",
            "GO_Mac_Mod_ContraX.3.zip"
        ],
        downloadSizeMB: 1838,
        diskSizeMB: 3100,
        markers: [
            // part 1
            "00_INI.big",
            "01_AI.big",
            "02_GameData.big",
            "03_W3D.big",
            "05_Terrain.big",
            "06_Window.big",
            "08_Textures.big",
            "11_HotkeysOriginal_English.big",
            "12_ControlBarPro.big",
            "13_CameosHD.big",
            "GenArial.ttf",
            "Install_Final.bmp",
            // part 2
            "07_Maps.big",
            "09_MusicEnhanced.big",
            "50_Textures.big",
            "51_ControlBarPro.big",
            "52_Patch1.big",
            "53_HotkeysOriginal_English.big",
            "54_CameosHD.big",
            "55_DisableFogEffects.big",
            // part 3
            "04_Audio.big",
            "10_UnitVoicesEnglish.big"
        ]
    )

    private static let releaseAssetsBase =
        "https://github.com/Okladnoj/GeneralsOnline-MacPatch/releases/latest/download"

    // A mod inherits the Zero Hour install, user data and executable; only content
    // location and presentation differ.
    private static func zeroHourMod(
        id: GameID,
        displayName: String,
        shortName: String,
        dirName: String,
        version: String,
        assetNames: [String],
        downloadSizeMB: Int,
        diskSizeMB: Int,
        markers: [String]
    ) -> GameProfile {
        let base = GameProfile.zeroHour

        return GameProfile(
            id: id,
            displayName: displayName,
            shortName: shortName,
            logsArchivePrefix: base.logsArchivePrefix,
            theme: .contra,
            userDataDirName: base.userDataDirName,
            requiredArchive: base.requiredArchive,
            forbiddenArchive: base.forbiddenArchive,
            bundleName: base.bundleName,
            executableName: base.executableName,
            patchAssetsDirName: base.patchAssetsDirName,
            patchMarkers: base.patchMarkers,
            localeLanguages: base.localeLanguages,
            localeRequiredFiles: base.localeRequiredFiles,
            localeDirName: base.localeDirName,
            localeFileName: base.localeFileName,
            onlineSettingsRelativePath: base.onlineSettingsRelativePath,
            supportedSettings: base.supportedSettings,
            mod: ModSpec(
                baseGameID: base.id,
                dirName: dirName,
                version: version,
                downloadURLs: assetNames.compactMap { URL(string: "\(releaseAssetsBase)/\($0)") },
                downloadSizeMB: downloadSizeMB,
                diskSizeMB: diskSizeMB,
                markers: [ModSpec.configFileName] + markers
            )
        )
    }

    // Chronological order: Generals, then Zero Hour, then mods.
    static let baseGames: [GameProfile] = [.generals, .zeroHour]

    static let mods: [GameProfile] = [.contra007, .contra008, .contra009, .contraX]

    static let all: [GameProfile] = baseGames + mods

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
