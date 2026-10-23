import Foundation
import Combine

struct HotkeyPresetPackage: Equatable {
    let packageVersion: Int
    let downloadURLs: [URL]
}

private struct HotkeyPresetCatalogPayload: Decodable {
    let version: Int
    let packageVersion: Int
    let urls: [String]
}

final class HotkeyPresetCatalog: ObservableObject, ApiResource {
    static let shared = HotkeyPresetCatalog()

    private static let schemaVersion = 1
    private static let downloadHost = "github.com"

    let resourceName = "hotkey-presets.json"

    @Published private(set) var package: HotkeyPresetPackage?

    private init() {}

    func reload() throws {
        let payload = try ApiCache.load(resourceName, decode: Self.decode)
        package = payload.flatMap(Self.package(from:))
    }

    private static func decode(_ data: Data) -> HotkeyPresetCatalogPayload? {
        guard let payload = try? JSONDecoder().decode(HotkeyPresetCatalogPayload.self, from: data),
              payload.version == schemaVersion else {
            return nil
        }

        return payload
    }

    private static func package(from payload: HotkeyPresetCatalogPayload) -> HotkeyPresetPackage? {
        guard payload.packageVersion >= 1, !payload.urls.isEmpty, payload.urls.allSatisfy(isAllowedDownload) else {
            return nil
        }

        return HotkeyPresetPackage(
            packageVersion: payload.packageVersion,
            downloadURLs: payload.urls.compactMap(URL.init(string:))
        )
    }

    private static func isAllowedDownload(_ string: String) -> Bool {
        guard let url = URL(string: string) else { return false }
        return url.scheme == "https" && url.host == downloadHost
    }
}
