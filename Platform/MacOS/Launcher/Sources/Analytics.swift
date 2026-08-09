import Foundation

enum Analytics {
    private static let measurementId =
        (Bundle.main.object(forInfoDictionaryKey: "GAMeasurementId") as? String) ?? ""
    private static let apiSecret =
        (Bundle.main.object(forInfoDictionaryKey: "GAApiSecret") as? String) ?? ""
    private static let collectEndpoint = "https://www.google-analytics.com/mp/collect"

    private static let optOutKey = "GA_ANALYTICS_OPT_OUT"
    private static let clientIdKey = "GA_CLIENT_ID"
    private static let reasonLimit = 100
    private static let deliveryTimeout: TimeInterval = 1.5

    static var isOptedOut: Bool {
        get { UserDefaults.standard.bool(forKey: optOutKey) }
        set { UserDefaults.standard.set(newValue, forKey: optOutKey) }
    }

    private static let clientId: String = {
        if let existing = UserDefaults.standard.string(forKey: clientIdKey) {
            return existing
        }
        let generated = UUID().uuidString
        UserDefaults.standard.set(generated, forKey: clientIdKey)
        return generated
    }()

    private static let sessionId: String = "\(Int(Date().timeIntervalSince1970))"

    private static var reportedAnnouncements: Set<String> = []

    private static let launcherVersion: String =
        (Bundle.main.object(forInfoDictionaryKey: "GOLauncherVersion") as? String) ?? "unknown"

    private static let userAgent: String = {
        let v = ProcessInfo.processInfo.operatingSystemVersion
        let osToken = "\(v.majorVersion)_\(v.minorVersion)_\(v.patchVersion)"
        return "Mozilla/5.0 (Macintosh; Intel Mac OS X \(osToken)) AppleWebKit/605.1.15 "
            + "(KHTML, like Gecko) Version/17.0 Safari/605.1.15 GeneralsLauncher/\(launcherVersion)"
    }()

    static func log(_ name: String, _ params: [String: Any] = [:], onDelivery: (() -> Void)? = nil) {
        guard !isOptedOut, measurementId.hasPrefix("G-"), !apiSecret.isEmpty else {
            onDelivery?()
            return
        }

        var enriched = params
        enriched["engagement_time_msec"] = 1
        enriched["session_id"] = sessionId
        enriched["launcher_version"] = launcherVersion
        enriched["game"] = GameProfile.selectedID.rawValue

        let payload: [String: Any] = [
            "client_id": clientId,
            "events": [["name": name, "params": enriched]]
        ]

        guard
            let body = try? JSONSerialization.data(withJSONObject: payload),
            let url = URL(string: "\(collectEndpoint)?measurement_id=\(measurementId)&api_secret=\(apiSecret)")
        else {
            onDelivery?()
            return
        }

        var request = URLRequest(url: url)
        request.httpMethod = "POST"
        request.httpBody = body
        request.setValue(userAgent, forHTTPHeaderField: "User-Agent")

        guard let onDelivery else {
            URLSession.shared.dataTask(with: request).resume()
            return
        }

        URLSession.shared.dataTask(with: request) { _, _, _ in
            DispatchQueue.main.async(execute: onDelivery)
        }.resume()
    }

    private static func shortReason(_ text: String) -> String {
        String(text.prefix(reasonLimit))
    }

    static func logOpen(uiLanguage: String, baseReady: Bool, hasStoredPath: Bool) {
        log("launcher_open", [
            "ui_language": uiLanguage,
            "os_version": ProcessInfo.processInfo.operatingSystemVersionString,
            "base_ready": baseReady.gaFlag,
            "has_stored_path": hasStoredPath.gaFlag
        ])
    }

    static func logSettingsSnapshot(_ vm: LauncherViewModel) {
        let profile = vm.selectedProfile
        let reportable: [(SettingKey, String, Any)] = [
            (.gameLanguage, "game_language", vm.gameLanguage),
            (.windowedEdgeScroll, "windowed_edge_scroll", vm.isWindowedEdgeScrollEnabled.gaFlag),
            (.showHotkeyLabels, "hotkey_labels", vm.showHotkeyLabels.gaFlag),
            (.wasdMapScroll, "wasd_map_scroll", vm.wasdMapScroll.gaFlag),
            (.limitFramerate, "limit_framerate", vm.limitFramerate.gaFlag),
            (.fpsLimit, "fps_limit", Int(vm.fpsLimit)),
            (.statsOverlay, "stats_overlay", vm.statsOverlay.gaFlag),
            (.altEndpoint, "alternative_endpoint", vm.useAlternativeEndpoint.gaFlag),
            (.verboseLogging, "verbose_logging", vm.verboseLogging.gaFlag),
            (.cameraSpeed, "camera_move_speed", vm.cameraMoveSpeed)
        ]

        var params: [String: Any] = ["ui_language": vm.selectedLanguage]
        for (key, name, value) in reportable where profile.supports(key) {
            params[name] = value
        }

        log("settings_snapshot", params)
    }

    static func logGameSwitched(from previous: GameID) {
        log("game_switched", ["from_game": previous.rawValue])
    }

    static func logSettingChanged(_ key: String, value: Any) {
        log("setting_changed_\(key)", ["setting_value": "\(value)"])
    }

    static func logTabSelected(_ tab: String) {
        log("tab_selected", ["tab": tab])
    }

    static func logGameLaunched(then finish: @escaping () -> Void) {
        var isFinished = false
        let finishOnce = {
            guard !isFinished else { return }
            isFinished = true
            finish()
        }

        log("game_launched", onDelivery: finishOnce)
        DispatchQueue.main.asyncAfter(deadline: .now() + deliveryTimeout, execute: finishOnce)
    }

    static func logLaunchFailed(reason: String) {
        log("launch_failed", ["reason": reason])
    }

    static func logPatchStarted(source: String) {
        log("patch_started", ["source": source])
    }

    static func logPatchFinished(seconds: Double) {
        log("patch_finished", ["duration_sec": Int(seconds.rounded())])
    }

    static func logPatchFailed(reason: String) {
        log("patch_failed", ["reason": shortReason(reason)])
    }

    static func logModInstallStarted(_ mod: GameID, parts: Int) {
        log("mod_install_started", ["mod": mod.rawValue, "parts": parts])
    }

    static func logModInstallFinished(_ mod: GameID, seconds: Double) {
        log("mod_install_finished", ["mod": mod.rawValue, "duration_sec": Int(seconds.rounded())])
    }

    static func logModInstallFailed(_ mod: GameID, stage: String, reason: String) {
        log("mod_install_failed", ["mod": mod.rawValue, "stage": stage, "reason": shortReason(reason)])
    }

    static func logModRemoved(_ mod: GameID) {
        log("mod_removed", ["mod": mod.rawValue])
    }

    static func logSteamDownloadStarted(needsSteamCMD: Bool) {
        log("steam_download_started", ["needs_steamcmd": needsSteamCMD.gaFlag])
    }

    static func logSteamGuardPrompted() {
        log("steam_guard_prompted")
    }

    static func logSteamDownloadFinished(seconds: Double) {
        log("steam_download_finished", ["duration_sec": Int(seconds.rounded())])
    }

    static func logSteamDownloadFailed(reason: String) {
        log("steam_download_failed", ["reason": shortReason(reason)])
    }

    static func logSteamDownloadCancelled(stage: String) {
        log("steam_download_cancelled", ["stage": stage])
    }

    static func logUpdateOffered(version: String) {
        log("update_offered", ["update_version": version])
    }

    static func logUpdateDismissed(version: String) {
        log("update_dismissed", ["update_version": version])
    }

    static func logUpdateOpened(version: String) {
        log("update_opened", ["update_version": version])
    }

    static func logAnnouncementShown(id: String) {
        guard !reportedAnnouncements.contains(id) else { return }

        reportedAnnouncements.insert(id)
        log("announcement_shown", ["announcement_id": id])
    }

    static func logAnnouncementClicked(id: String, url: URL) {
        log("announcement_clicked", [
            "announcement_id": id,
            "link_host": url.host ?? url.scheme ?? "unknown"
        ])
    }

    static func logLinkOpened(target: String, location: String) {
        log("link_opened", ["link_target": target, "link_location": location])
    }

    static func logFolderChosen(isValid: Bool) {
        log("folder_chosen", ["is_valid": isValid.gaFlag])
    }
}

private extension Bool {
    var gaFlag: Int { self ? 1 : 0 }
}
