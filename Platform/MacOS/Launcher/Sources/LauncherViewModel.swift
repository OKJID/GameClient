import SwiftUI
import AppKit
import Combine

class LauncherViewModel: ObservableObject {
    enum Tab: String, CaseIterable {
        case steam = "Steam"
        case local = "Local Archive"
    }

    struct ModConfirmation: Identifiable {
        enum Kind {
            case reinstall
            case remove
        }

        let id = UUID()
        let kind: Kind
        let profile: GameProfile
    }

    private static let activeTabKey = "ActiveTab"

    private static var restoredTab: Tab {
        guard let raw = UserDefaults.standard.string(forKey: activeTabKey),
              let tab = Tab(rawValue: raw) else {
            return .steam
        }

        return tab
    }

    @Published var activeTab: Tab = LauncherViewModel.restoredTab {
        didSet {
            UserDefaults.standard.set(activeTab.rawValue, forKey: LauncherViewModel.activeTabKey)
            invalidateValidation()
        }
    }
    @Published var selectedGameID: GameID = GameProfile.selectedID
    @Published var installPath: String = UserDefaults.standard.string(forKey: "GENERALS_INSTALL_PATH") ?? "" {
        didSet { invalidateValidation() }
    }
    @Published var isLaunching: Bool = false
    @Published var alertMessage: String? = nil
    @Published var steamUsername: String = ""
    @Published var steamPassword: String = ""
    @Published var isUpdateDismissed: Bool = false
    @Published var showPatchConfirmation: Bool = false
    @Published var modConfirmation: ModConfirmation? = nil
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
    var modInstaller = ModInstaller()
    var updateChecker = UpdateChecker()
    private var cancellables = Set<AnyCancellable>()
    private var isInitializing = true

    // Install validation touches hundreds of files (patch markers, locales, mod
    // markers). SwiftUI re-reads these on every redraw, so results are cached until
    // the tab, the path or a finished download changes them.
    private struct ModStatus {
        let installed: Bool
        let damaged: Bool
        let missingCount: Int

        static let unknown = ModStatus(installed: false, damaged: false, missingCount: 0)
    }

    private var validationEpoch: Int = 0
    private var validationToken: String = ""
    private var installDirCache: [GameID: URL?] = [:]
    private var baseReadyCache: [GameID: Bool] = [:]
    private var modStatusCache: [GameID: ModStatus] = [:]
    private var pathValidCache: Bool?

    init() {
        steamCMD.objectWillChange.sink { [weak self] _ in
            self?.objectWillChange.send()
        }.store(in: &cancellables)

        assetPatcher.objectWillChange.sink { [weak self] _ in
            self?.objectWillChange.send()
        }.store(in: &cancellables)

        modInstaller.objectWillChange.sink { [weak self] _ in
            self?.objectWillChange.send()
        }.store(in: &cancellables)

        updateChecker.$availableUpdate
            .receive(on: RunLoop.main)
            .sink { [weak self] _ in
                self?.objectWillChange.send()
            }
            .store(in: &cancellables)

        // Progress ticks must not drop the cache; only finished work changes the disk.
        steamCMD.$state
            .removeDuplicates()
            .sink { [weak self] state in
                guard !state.isRunning else { return }
                self?.invalidateValidation()
            }
            .store(in: &cancellables)

        assetPatcher.$state
            .removeDuplicates()
            .sink { [weak self] state in
                guard !state.isRunning else { return }
                self?.invalidateValidation()
            }
            .store(in: &cancellables)

        modInstaller.$states
            .map { states in states.values.contains { $0.isRunning } }
            .removeDuplicates()
            .sink { [weak self] isRunning in
                guard !isRunning else { return }
                self?.invalidateValidation()
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
        syncValidationCache()
        if let cached = pathValidCache { return cached }

        let valid = !installPath.isEmpty && _validateGameFolder(at: URL(fileURLWithPath: installPath))
        pathValidCache = valid
        return valid
    }

    // MARK: - Validation cache

    private func invalidateValidation() {
        validationEpoch += 1
    }

    private func syncValidationCache() {
        let token = "\(activeTab.rawValue)|\(effectiveInstallPath)|\(validationEpoch)"
        guard token != validationToken else { return }

        validationToken = token
        installDirCache.removeAll()
        baseReadyCache.removeAll()
        modStatusCache.removeAll()
        pathValidCache = nil
    }

    // Falls back to the base game while a mod cannot run, so a stale selection never
    // blocks the launcher.
    var selectedProfile: GameProfile {
        let profile = GameProfile.profile(for: selectedGameID)
        guard profile.isMod, !isBaseReady(for: profile) else { return profile }
        return profile.baseProfile
    }

    func installDirectory(for profile: GameProfile) -> URL? {
        syncValidationCache()
        if let cached = installDirCache[profile.id] { return cached }

        let resolved = _resolveInstallDirectory(for: profile)
        installDirCache[profile.id] = resolved
        return resolved
    }

    private func _resolveInstallDirectory(for profile: GameProfile) -> URL? {
        switch activeTab {
        case .steam:
            return [steamCMD.assetsDir, steamCMD.baseGameDir].first { profile.matchesInstall(at: $0) }
        case .local:
            guard !installPath.isEmpty else { return nil }
            return GameProfile.installDirectory(for: profile.id, under: URL(fileURLWithPath: installPath))
        }
    }

    var patchTargets: [PatchTarget] {
        GameProfile.baseGames.compactMap { profile in
            guard let directory = installDirectory(for: profile) else { return nil }
            return PatchTarget(profile: profile, directory: directory)
        }
    }

    // MARK: - Mods

    var installRootURL: URL? {
        let path = effectiveInstallPath
        guard !path.isEmpty else { return nil }
        return URL(fileURLWithPath: path)
    }

    func isBaseReady(for profile: GameProfile) -> Bool {
        syncValidationCache()

        let base = profile.baseProfile
        if let cached = baseReadyCache[base.id] { return cached }

        let ready = installDirectory(for: base)
            .map { assetPatcher.isCommunityPatchInstalled(base, at: $0) } ?? false
        baseReadyCache[base.id] = ready
        return ready
    }

    // Mods are meaningless without a launchable base game, so the whole section stays
    // hidden until the base game passes validation.
    var availableMods: [GameProfile] {
        GameProfile.mods.filter { isBaseReady(for: $0) }
    }

    // Set while a package is being fetched, so every screen can block actions that
    // would kill the download (launching quits the launcher, patching writes in parallel).
    var installingMod: GameProfile? {
        guard let id = modInstaller.runningModID else { return nil }
        return GameProfile.mods.first { $0.id == id }
    }

    func isModInstalled(_ profile: GameProfile) -> Bool {
        modStatus(profile).installed
    }

    // User deleted or corrupted files after a successful install.
    func isModDamaged(_ profile: GameProfile) -> Bool {
        modStatus(profile).damaged
    }

    func missingModFileCount(_ profile: GameProfile) -> Int {
        modStatus(profile).missingCount
    }

    private func modStatus(_ profile: GameProfile) -> ModStatus {
        syncValidationCache()
        if let cached = modStatusCache[profile.id] { return cached }

        let status = _resolveModStatus(profile)
        modStatusCache[profile.id] = status
        return status
    }

    private func _resolveModStatus(_ profile: GameProfile) -> ModStatus {
        guard profile.isMod, let root = installRootURL,
              let directory = profile.modDirectory(installRoot: root) else {
            return .unknown
        }

        let missing = profile.missingModFiles(installRoot: root)
        guard !missing.isEmpty else {
            return ModStatus(installed: true, damaged: false, missingCount: 0)
        }

        let exists = FileManager.default.fileExists(atPath: directory.path)
        return ModStatus(installed: false, damaged: exists, missingCount: missing.count)
    }

    func modState(_ profile: GameProfile) -> ModInstallState {
        let state = modInstaller.state(for: profile.id)
        guard case .idle = state, isModInstalled(profile) else { return state }
        return .completed
    }

    // Both actions wipe the mod directory before doing anything else, and ContraX alone is
    // 3 GB over three parts, so they ask first.
    func requestModReinstall(_ profile: GameProfile) {
        modConfirmation = ModConfirmation(kind: .reinstall, profile: profile)
    }

    func requestModRemoval(_ profile: GameProfile) {
        modConfirmation = ModConfirmation(kind: .remove, profile: profile)
    }

    func confirmModAction(_ confirmation: ModConfirmation) {
        switch confirmation.kind {
        case .reinstall: installMod(confirmation.profile)
        case .remove: removeMod(confirmation.profile)
        }
    }

    func installMod(_ profile: GameProfile) {
        guard let root = installRootURL else {
            alertMessage = L10n.alerts.folderNotSelected
            return
        }

        modInstaller.install(profile, installRoot: root)
    }

    func removeMod(_ profile: GameProfile) {
        guard let root = installRootURL else { return }
        modInstaller.remove(profile, installRoot: root)
    }

    var isSteamPatchReady: Bool {
        assetPatcher.isCommunityPatchInstalled(.zeroHour, at: steamCMD.assetsDir)
    }

    var isPatchReady: Bool {
        isBaseReady(for: selectedProfile)
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
        guard !modInstaller.isBusy else { return false }

        if selectedProfile.isMod {
            guard isModInstalled(selectedProfile), !isModDamaged(selectedProfile) else { return false }
        }

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
        guard !modInstaller.isBusy else { return }

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
        guard !modInstaller.isBusy else { return }

        switch activeTab {
        case .steam:
            confirmPatching()
        case .local:
            showPatchConfirmation = true
        }
    }

    func confirmPatching() {
        guard !modInstaller.isBusy else { return }

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
            alertMessage = String(format: L10n.alerts.binaryNotFound, profile.displayName, executableURL.path)
            isLaunching = false
            return
        }

        var arguments: [String] = []
        if profile.isMod {
            guard let root = installRootURL,
                  let modDir = profile.modDirectory(installRoot: root) else {
                alertMessage = L10n.alerts.modFolderMissing
                isLaunching = false
                return
            }

            let configURL = modDir.appendingPathComponent(ModSpec.configFileName)
            guard FileManager.default.fileExists(atPath: configURL.path) else {
                alertMessage = String(format: L10n.alerts.modConfigMissing, profile.displayName, ModSpec.configFileName)
                isLaunching = false
                return
            }

            arguments = ["-mod", configURL.path]
        }

        let task = Process()
        task.executableURL = executableURL
        task.currentDirectoryURL = executableURL.deletingLastPathComponent()
        task.arguments = arguments

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
            alertMessage = String(format: L10n.alerts.launchFailed, error.localizedDescription)
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
