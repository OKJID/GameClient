import SwiftUI
import AppKit
import Combine

class LauncherViewModel: ObservableObject {
    enum Tab: String, CaseIterable {
        case steam = "Steam"
        case local = "Local Archive"
    }

    @Published var activeTab: Tab = .steam
    @Published var selectedGameID: GameID = GameProfile.selectedID
    @Published var installPath: String = UserDefaults.standard.string(forKey: "GENERALS_INSTALL_PATH") ?? ""
    @Published var isLaunching: Bool = false
    @Published var alertMessage: String? = nil
    @Published var steamUsername: String = ""
    @Published var steamPassword: String = ""
    @Published var isUpdateDismissed: Bool = false
    @Published var showPatchConfirmation: Bool = false
    @Published var selectedLanguage: String = L10n.current
    @Published var isWindowedEdgeScrollEnabled: Bool = false {
        didSet {
            let valStr = isWindowedEdgeScrollEnabled ? "yes" : "no"
            OptionsIniHelper.writeValues([
                "ScreenEdgeScrollEnabledInWindowedApp": valStr,
                "CursorCaptureEnabledInWindowedGame": isWindowedEdgeScrollEnabled ? "yes" : "no"
            ])
            reportSettingChange("windowed_edge_scroll", isWindowedEdgeScrollEnabled)
        }
    }
    @Published var showHotkeyLabels: Bool = SettingsDefaults.showHotkeyLabels {
        didSet {
            OptionsIniHelper.writeValue(value: showHotkeyLabels ? "yes" : "no", forKey: "ShowHotKeyLabels")
            reportSettingChange("hotkey_labels", showHotkeyLabels)
        }
    }
    @Published var gameLanguage: String = SettingsDefaults.gameLanguage {
        didSet {
            guard !isInitializing else { return }
            OptionsIniHelper.writeValue(value: gameLanguage, forKey: "Language")
            reportSettingChange("game_language", gameLanguage)
        }
    }
    
    // settings.json camera settings
    @Published var cameraMinHeight: Double = SettingsDefaults.cameraMinHeight {
        didSet { saveSettings() }
    }
    @Published var cameraMaxHeight: Double = SettingsDefaults.cameraMaxHeight {
        didSet { saveSettings() }
    }
    @Published var cameraMoveSpeed: Double = SettingsDefaults.cameraMoveSpeed {
        didSet { saveSettings() }
    }
    
    // settings.json render settings
    @Published var limitFramerate: Bool = SettingsDefaults.limitFramerate {
        didSet {
            saveSettings()
            reportSettingChange("limit_framerate", limitFramerate)
        }
    }
    @Published var fpsLimit: Double = SettingsDefaults.fpsLimit {
        didSet { saveSettings() }
    }
    @Published var statsOverlay: Bool = SettingsDefaults.statsOverlay {
        didSet {
            saveSettings()
            reportSettingChange("stats_overlay", statsOverlay)
        }
    }
    
    // settings.json network settings
    @Published var useAlternativeEndpoint: Bool = SettingsDefaults.useAlternativeEndpoint {
        didSet {
            saveSettings()
            reportSettingChange("alternative_endpoint", useAlternativeEndpoint)
        }
    }
    
    // Options.ini debug settings
    @Published var verboseLogging: Bool = SettingsDefaults.verboseLogging {
        didSet {
            OptionsIniHelper.writeValue(value: verboseLogging ? "yes" : "no", forKey: "VerboseEngineLogging")
            reportSettingChange("verbose_logging", verboseLogging)
        }
    }

    var steamCMD = SteamCMDManager()
    var assetPatcher = AssetPatcher()
    var updateChecker = UpdateChecker()
    private var cancellables = Set<AnyCancellable>()
    private var isInitializing = true

    init() {
        steamCMD.objectWillChange.sink { [weak self] _ in
            self?.objectWillChange.send()
        }.store(in: &cancellables)

        assetPatcher.objectWillChange.sink { [weak self] _ in
            self?.objectWillChange.send()
        }.store(in: &cancellables)

        updateChecker.$availableUpdate
            .receive(on: RunLoop.main)
            .sink { [weak self] _ in
                self?.objectWillChange.send()
            }
            .store(in: &cancellables)

        if let username = KeychainHelper.savedUsername() {
            steamUsername = username
            steamPassword = KeychainHelper.load(account: username) ?? ""
        }

        loadSettings()

        self.isInitializing = false
        updateChecker.startPeriodicChecks()

        Analytics.logOpen(uiLanguage: selectedLanguage)
        Analytics.logSettingsSnapshot(self)
    }

    private func loadSettings() {
        let path = OptionsIniHelper.optionsFilePath
        if !FileManager.default.fileExists(atPath: path.path) {
            OptionsIniHelper.writeValues([
                "ScreenEdgeScrollEnabledInWindowedApp": "no",
                "CursorCaptureEnabledInWindowedGame": "yes"
            ])
        }

        isWindowedEdgeScrollEnabled = OptionsIniHelper.readValue(forKey: "ScreenEdgeScrollEnabledInWindowedApp") == "yes"
        showHotkeyLabels = OptionsIniHelper.readValue(forKey: "ShowHotKeyLabels") == "yes"
        verboseLogging = OptionsIniHelper.readValue(forKey: "VerboseEngineLogging") == "yes"
        gameLanguage = OptionsIniHelper.readValue(forKey: "Language") ?? SettingsDefaults.gameLanguage

        loadOnlineSettings()
    }

    private func loadOnlineSettings() {
        guard let json = SettingsJsonHelper.readSettings() else {
            return
        }

        let camera = json["camera"] as? [String: Any]
        cameraMinHeight = camera?["min_height"] as? Double ?? SettingsDefaults.cameraMinHeight
        cameraMaxHeight = camera?["max_height_only_when_lobby_host"] as? Double ?? SettingsDefaults.cameraMaxHeight
        cameraMoveSpeed = camera?["move_speed_ratio"] as? Double ?? SettingsDefaults.cameraMoveSpeed

        let render = json["render"] as? [String: Any]
        limitFramerate = render?["limit_framerate"] as? Bool ?? SettingsDefaults.limitFramerate
        if let fps = render?["fps_limit"] as? Int {
            fpsLimit = Double(fps)
        } else if let fpsDouble = render?["fps_limit"] as? Double {
            fpsLimit = fpsDouble
        } else {
            fpsLimit = SettingsDefaults.fpsLimit
        }
        statsOverlay = render?["stats_overlay"] as? Bool ?? SettingsDefaults.statsOverlay

        let network = json["network"] as? [String: Any]
        useAlternativeEndpoint = network?["use_alternative_endpoint"] as? Bool ?? SettingsDefaults.useAlternativeEndpoint
    }

    func selectGame(_ id: GameID) {
        guard id != selectedGameID else {
            return
        }

        let previous = selectedGameID
        isInitializing = true
        GameProfile.selectedID = id
        selectedGameID = id
        loadSettings()
        isInitializing = false

        Analytics.logGameSwitched(from: previous)
        Analytics.logSettingsSnapshot(self)
    }

    func saveCredentials() {
        guard !steamUsername.isEmpty, !steamPassword.isEmpty else { return }
        KeychainHelper.save(account: steamUsername, password: steamPassword)
    }

    private func reportSettingChange(_ key: String, _ value: Any) {
        guard !isInitializing else { return }
        Analytics.logSettingChanged(key, value: value)
    }

    private func saveSettings() {
        guard !isInitializing else { return }
        SettingsJsonHelper.writeSettings(
            cameraMinHeight: cameraMinHeight,
            cameraMaxHeight: cameraMaxHeight,
            cameraMoveSpeed: cameraMoveSpeed,
            limitFramerate: limitFramerate,
            fpsLimit: Int(fpsLimit),
            statsOverlay: statsOverlay,
            useAlternativeEndpoint: useAlternativeEndpoint
        )
    }

    func resetAllSettings() {
        isWindowedEdgeScrollEnabled = SettingsDefaults.isWindowedEdgeScrollEnabled
        showHotkeyLabels = SettingsDefaults.showHotkeyLabels
        gameLanguage = SettingsDefaults.gameLanguage
        cameraMinHeight = SettingsDefaults.cameraMinHeight
        cameraMaxHeight = SettingsDefaults.cameraMaxHeight
        cameraMoveSpeed = SettingsDefaults.cameraMoveSpeed
        limitFramerate = SettingsDefaults.limitFramerate
        fpsLimit = SettingsDefaults.fpsLimit
        statsOverlay = SettingsDefaults.statsOverlay
        useAlternativeEndpoint = SettingsDefaults.useAlternativeEndpoint
        verboseLogging = SettingsDefaults.verboseLogging
    }

    var isPathValid: Bool {
        guard !installPath.isEmpty else { return false }
        return _validateGameFolder(at: URL(fileURLWithPath: installPath))
    }

    var selectedProfile: GameProfile {
        GameProfile.profile(for: selectedGameID)
    }

    func installDirectory(for profile: GameProfile) -> URL? {
        switch activeTab {
        case .steam:
            return [steamCMD.assetsDir, steamCMD.baseGameDir].first { profile.matchesInstall(at: $0) }
        case .local:
            guard !installPath.isEmpty else { return nil }
            return GameProfile.installDirectory(for: profile.id, under: URL(fileURLWithPath: installPath))
        }
    }

    var patchTargets: [PatchTarget] {
        GameProfile.all.compactMap { profile in
            guard let directory = installDirectory(for: profile) else { return nil }
            return PatchTarget(profile: profile, directory: directory)
        }
    }

    var isSteamPatchReady: Bool {
        assetPatcher.isCommunityPatchInstalled(.zeroHour, at: steamCMD.assetsDir)
    }

    var isPatchReady: Bool {
        let profile = GameProfile.current
        guard let directory = installDirectory(for: profile) else { return false }
        return assetPatcher.isCommunityPatchInstalled(profile, at: directory)
    }

    var needsPatching: Bool {
        switch activeTab {
        case .steam:
            return steamCMD.areAssetsValid && !isPatchReady
                && !steamCMD.state.isRunning && !assetPatcher.state.isRunning
        case .local:
            return isPathValid && !isPatchReady && !assetPatcher.state.isRunning
        }
    }

    var canLaunch: Bool {
        switch activeTab {
        case .steam: return steamCMD.areAssetsValid && isPatchReady
            && !steamCMD.state.isRunning && !assetPatcher.state.isRunning
        case .local: return isPathValid && isPatchReady && !assetPatcher.state.isRunning
        }
    }

    var effectiveInstallPath: String {
        switch activeTab {
        case .steam: return steamCMD.installDir.path
        case .local: return installPath
        }
    }

    func chooseFolder() {
        let panel = NSOpenPanel()
        panel.canChooseFiles = false
        panel.canChooseDirectories = true
        panel.allowsMultipleSelection = false
        panel.message = NSLocalizedString("Select the Windows Game Folder (containing .big files)", comment: "")

        if panel.runModal() == .OK, let url = panel.url {
            DispatchQueue.main.async {
                self.installPath = url.path
                UserDefaults.standard.set(self.installPath, forKey: "GENERALS_INSTALL_PATH")
                self.objectWillChange.send()
            }
        }
    }

    func requestPatching() {
        switch activeTab {
        case .steam:
            confirmPatching()
        case .local:
            showPatchConfirmation = true
        }
    }

    func confirmPatching() {
        Analytics.logPatchStarted(source: activeTab.rawValue)

        let rootDir: URL
        switch activeTab {
        case .steam: rootDir = steamCMD.installDir
        case .local: rootDir = URL(fileURLWithPath: installPath)
        }

        assetPatcher.startPatching(rootDir: rootDir, targets: patchTargets)
    }

    func launchGame() {
        guard canLaunch else { return }
        isLaunching = true

        let profile = selectedProfile
        guard let executableURL = Bundle.main.executableURL?
            .deletingLastPathComponent()
            .appendingPathComponent(profile.executableName) else {
            isLaunching = false
            return
        }

        guard FileManager.default.fileExists(atPath: executableURL.path) else {
            alertMessage = "\(profile.displayName) binary not found at \(executableURL.path)"
            isLaunching = false
            return
        }

        let task = Process()
        task.executableURL = executableURL
        task.currentDirectoryURL = executableURL.deletingLastPathComponent()

        var env = ProcessInfo.processInfo.environment
        env["GENERALS_INSTALL_PATH"] = effectiveInstallPath
        task.environment = env

        do {
            try task.run()

            Analytics.logGameLaunched()

            DispatchQueue.global().async {
                Thread.sleep(forTimeInterval: 0.5)
                DispatchQueue.main.async {
                    if let app = NSRunningApplication(processIdentifier: task.processIdentifier) {
                        app.activate(options: .activateIgnoringOtherApps)
                    }
                    NSApplication.shared.terminate(nil)
                }
            }
        } catch {
            alertMessage = "Failed to launch game: \(error.localizedDescription)"
            isLaunching = false
        }
    }

    private func _validateGameFolder(at url: URL) -> Bool {
        let fm = FileManager.default
        guard let items = try? fm.contentsOfDirectory(at: url, includingPropertiesForKeys: [.isDirectoryKey], options: .skipsHiddenFiles) else {
            return false
        }

        var hasZH = false
        var hasBase = false

        for itemURL in items {
            guard let isDir = try? itemURL.resourceValues(forKeys: [.isDirectoryKey]).isDirectory, isDir else { continue }
            guard let subItems = try? fm.contentsOfDirectory(atPath: itemURL.path) else { continue }

            if subItems.contains(where: { $0.lowercased() == "inizh.big" }) { hasZH = true }
            if subItems.contains(where: { $0.lowercased() == "ini.big" }) { hasBase = true }

            if hasZH && hasBase { return true }
        }

        return false
    }
}
