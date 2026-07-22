import Foundation

enum Analytics {
    private static let measurementId =
        (Bundle.main.object(forInfoDictionaryKey: "GAMeasurementId") as? String) ?? ""
    private static let apiSecret =
        (Bundle.main.object(forInfoDictionaryKey: "GAApiSecret") as? String) ?? ""
    private static let collectEndpoint = "https://www.google-analytics.com/mp/collect"

    private static let optOutKey = "GA_ANALYTICS_OPT_OUT"
    private static let clientIdKey = "GA_CLIENT_ID"

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

    private static let launcherVersion: String =
        (Bundle.main.object(forInfoDictionaryKey: "GOLauncherVersion") as? String) ?? "unknown"

    static func log(_ name: String, _ params: [String: Any] = [:]) {
        guard !isOptedOut, measurementId.hasPrefix("G-"), !apiSecret.isEmpty else { return }

        var enriched = params
        enriched["engagement_time_msec"] = 1
        enriched["launcher_version"] = launcherVersion
        enriched["game"] = GameProfile.selectedID.rawValue

        let payload: [String: Any] = [
            "client_id": clientId,
            "events": [["name": name, "params": enriched]]
        ]

        guard
            let body = try? JSONSerialization.data(withJSONObject: payload),
            let url = URL(string: "\(collectEndpoint)?measurement_id=\(measurementId)&api_secret=\(apiSecret)")
        else { return }

        var request = URLRequest(url: url)
        request.httpMethod = "POST"
        request.httpBody = body
        URLSession.shared.dataTask(with: request).resume()
    }

    static func logOpen(uiLanguage: String) {
        log("launcher_open", [
            "ui_language": uiLanguage,
            "os_version": ProcessInfo.processInfo.operatingSystemVersionString
        ])
    }

    static func logSettingsSnapshot(_ vm: LauncherViewModel) {
        let profile = vm.selectedProfile
        let reportable: [(SettingKey, String, Any)] = [
            (.gameLanguage, "game_language", vm.gameLanguage),
            (.windowedEdgeScroll, "windowed_edge_scroll", vm.isWindowedEdgeScrollEnabled.gaFlag),
            (.showHotkeyLabels, "hotkey_labels", vm.showHotkeyLabels.gaFlag),
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

    static func logGameLaunched() {
        log("game_launched")
    }

    static func logPatchStarted(source: String) {
        log("patch_started", ["source": source])
    }
}

private extension Bool {
    var gaFlag: Int { self ? 1 : 0 }
}
