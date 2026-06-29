import SwiftUI
import AppKit
import Combine

class LauncherViewModel: ObservableObject {
    enum Tab: String, CaseIterable {
        case steam = "Steam"
        case local = "Local Archive"
    }

    @Published var activeTab: Tab = .steam
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
        }
    }
    @Published var showHotkeyLabels: Bool = SettingsDefaults.showHotkeyLabels {
        didSet {
            OptionsIniHelper.writeValue(value: showHotkeyLabels ? "yes" : "no", forKey: "ShowHotKeyLabels")
        }
    }
    @Published var gameLanguage: String = SettingsDefaults.gameLanguage {
        didSet {
            guard !isInitializing else { return }
            OptionsIniHelper.writeValue(value: gameLanguage, forKey: "Language")
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
        didSet { saveSettings() }
    }
    @Published var fpsLimit: Double = SettingsDefaults.fpsLimit {
        didSet { saveSettings() }
    }
    @Published var statsOverlay: Bool = SettingsDefaults.statsOverlay {
        didSet { saveSettings() }
    }
    
    // settings.json network settings
    @Published var useAlternativeEndpoint: Bool = SettingsDefaults.useAlternativeEndpoint {
        didSet { saveSettings() }
    }
    
    // settings.json debug settings
    @Published var verboseLogging: Bool = SettingsDefaults.verboseLogging {
        didSet { saveSettings() }
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

        let path = OptionsIniHelper.optionsFilePath
        if !FileManager.default.fileExists(atPath: path.path) {
            OptionsIniHelper.writeValues([
                "ScreenEdgeScrollEnabledInWindowedApp": "no",
                "CursorCaptureEnabledInWindowedGame": "yes"
            ])
        }
        self.isWindowedEdgeScrollEnabled = OptionsIniHelper.readValue(forKey: "ScreenEdgeScrollEnabledInWindowedApp") == "yes"
        self.showHotkeyLabels = OptionsIniHelper.readValue(forKey: "ShowHotKeyLabels") == "yes"
        self.gameLanguage = OptionsIniHelper.readValue(forKey: "Language") ?? SettingsDefaults.gameLanguage
        
        // Load settings.json
        if let json = SettingsJsonHelper.readSettings() {
            let camera = json["camera"] as? [String: Any]
            self.cameraMinHeight = camera?["min_height"] as? Double ?? SettingsDefaults.cameraMinHeight
            self.cameraMaxHeight = camera?["max_height_only_when_lobby_host"] as? Double ?? SettingsDefaults.cameraMaxHeight
            self.cameraMoveSpeed = camera?["move_speed_ratio"] as? Double ?? SettingsDefaults.cameraMoveSpeed
            
            let render = json["render"] as? [String: Any]
            self.limitFramerate = render?["limit_framerate"] as? Bool ?? SettingsDefaults.limitFramerate
            if let fps = render?["fps_limit"] as? Int {
                self.fpsLimit = Double(fps)
            } else if let fpsDouble = render?["fps_limit"] as? Double {
                self.fpsLimit = fpsDouble
            } else {
                self.fpsLimit = SettingsDefaults.fpsLimit
            }
            self.statsOverlay = render?["stats_overlay"] as? Bool ?? SettingsDefaults.statsOverlay
            
            let network = json["network"] as? [String: Any]
            self.useAlternativeEndpoint = network?["use_alternative_endpoint"] as? Bool ?? SettingsDefaults.useAlternativeEndpoint
            
            let debug = json["debug"] as? [String: Any]
            self.verboseLogging = debug?["verbose_logging"] as? Bool ?? SettingsDefaults.verboseLogging
        }
        
        self.isInitializing = false
        updateChecker.startPeriodicChecks()
    }

    func saveCredentials() {
        guard !steamUsername.isEmpty, !steamPassword.isEmpty else { return }
        KeychainHelper.save(account: steamUsername, password: steamPassword)
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
            useAlternativeEndpoint: useAlternativeEndpoint,
            verboseLogging: verboseLogging
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

    var zhDirectoryURL: URL? {
        guard !installPath.isEmpty else { return nil }
        return AssetPatcher.findZHDirectory(at: URL(fileURLWithPath: installPath))
    }

    var isSteamPatchReady: Bool {
        return assetPatcher.isCommunityPatchInstalled(at: steamCMD.assetsDir)
    }

    var isPatchReady: Bool {
        switch activeTab {
        case .steam: return isSteamPatchReady
        case .local:
            guard let zhDir = zhDirectoryURL else { return false }
            return assetPatcher.isCommunityPatchInstalled(at: zhDir)
        }
    }

    var needsPatching: Bool {
        switch activeTab {
        case .steam:
            return steamCMD.areAssetsValid && !isSteamPatchReady
                && !steamCMD.state.isRunning && !assetPatcher.state.isRunning
        case .local:
            return isPathValid && !isPatchReady && !assetPatcher.state.isRunning
        }
    }

    var canLaunch: Bool {
        switch activeTab {
        case .steam: return steamCMD.areAssetsValid && isSteamPatchReady
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
        switch activeTab {
        case .steam:
            assetPatcher.startPatching(rootDir: steamCMD.installDir, zhDir: steamCMD.assetsDir)
        case .local:
            guard let zhDir = zhDirectoryURL else { return }
            let rootURL = URL(fileURLWithPath: installPath)
            assetPatcher.startPatching(rootDir: rootURL, zhDir: zhDir)
        }
    }

    func launchGame() {
        guard canLaunch else { return }
        isLaunching = true

        guard let executableURL = Bundle.main.executableURL?.deletingLastPathComponent().appendingPathComponent("GeneralsOnlineZH") else {
            isLaunching = false
            return
        }

        guard FileManager.default.fileExists(atPath: executableURL.path) else {
            alertMessage = "Engine binary not found at \(executableURL.path)"
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
