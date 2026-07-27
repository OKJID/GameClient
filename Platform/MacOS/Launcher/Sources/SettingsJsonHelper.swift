import Foundation

struct SettingsJsonHelper {
    static var settingsFilePath: URL? {
        GameProfile.current.onlineSettingsFileURL
    }

    static func readSettings() -> [String: Any]? {
        guard let path = settingsFilePath else {
            return nil
        }

        guard FileManager.default.fileExists(atPath: path.path),
              let data = try? Data(contentsOf: path),
              let json = try? JSONSerialization.jsonObject(with: data, options: []) as? [String: Any] else {
            return nil
        }
        return json
    }
    
    static func writeSettings(
        cameraMinHeight: Double,
        cameraMaxHeight: Double,
        cameraMoveSpeed: Double,
        limitFramerate: Bool,
        fpsLimit: Int,
        statsOverlay: Bool,
        useAlternativeEndpoint: Bool
    ) {
        guard let path = settingsFilePath else {
            return
        }

        let dir = path.deletingLastPathComponent()

        if !FileManager.default.fileExists(atPath: dir.path) {
            try? FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true, attributes: nil)
        }
        
        var json: [String: Any] = [:]
        if FileManager.default.fileExists(atPath: path.path),
           let data = try? Data(contentsOf: path),
           let existingJson = try? JSONSerialization.jsonObject(with: data, options: []) as? [String: Any] {
            json = existingJson
        }
        
        var camera = json["camera"] as? [String: Any] ?? [:]
        camera["min_height"] = cameraMinHeight
        camera["max_height_only_when_lobby_host"] = cameraMaxHeight
        camera["move_speed_ratio"] = cameraMoveSpeed
        json["camera"] = camera
        
        var render = json["render"] as? [String: Any] ?? [:]
        render["limit_framerate"] = limitFramerate
        render["fps_limit"] = fpsLimit
        render["stats_overlay"] = statsOverlay
        json["render"] = render
        
        var network = json["network"] as? [String: Any] ?? [:]
        network["use_alternative_endpoint"] = useAlternativeEndpoint
        if network["http_version"] == nil {
            network["http_version"] = 0
        }
        json["network"] = network

        if let data = try? JSONSerialization.data(withJSONObject: json, options: [.prettyPrinted]),
           let jsonString = String(data: data, encoding: .utf8) {
            try? jsonString.write(to: path, atomically: true, encoding: .utf8)
        }
    }
}
