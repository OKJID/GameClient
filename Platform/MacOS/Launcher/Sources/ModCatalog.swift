import Foundation
import Combine

struct ModCatalogEntry: Decodable {
    let id: String
    let baseGame: String
    let displayName: String
    let shortName: String
    let theme: String
    let dirName: String
    let packageVersion: Int
    let urls: [String]
    let downloadSizeMB: Int
    let diskSizeMB: Int
    let markers: [String]
}

private struct ModCatalogPayload: Decodable {
    let version: Int
    let mods: [LenientEntry]
}

private struct LenientEntry: Decodable {
    let entry: ModCatalogEntry?

    init(from decoder: Decoder) throws {
        entry = try? ModCatalogEntry(from: decoder)
    }
}

final class ModCatalog: ObservableObject, ApiResource {
    static let shared = ModCatalog()

    private static let schemaVersion = 1

    let resourceName = "mods.json"

    @Published private(set) var mods: [GameProfile] = []

    private init() {}

    func reload() throws {
        let payload = try ApiCache.load(resourceName, decode: Self.decode)
        mods = Self.profiles(from: payload?.mods.compactMap(\.entry) ?? [])
    }

    private static func decode(_ data: Data) -> ModCatalogPayload? {
        guard let payload = try? JSONDecoder().decode(ModCatalogPayload.self, from: data),
              payload.version == schemaVersion else {
            return nil
        }

        return payload
    }

    private static func profiles(from entries: [ModCatalogEntry]) -> [GameProfile] {
        var usedIDs = Set(GameProfile.baseGames.map(\.id.rawValue))

        return entries
            .filter(\.isAcceptable)
            .filter { usedIDs.insert($0.id).inserted }
            .map(profile)
    }

    private static func profile(from entry: ModCatalogEntry) -> GameProfile {
        GameProfile.zeroHourMod(
            id: GameID(rawValue: entry.id),
            displayName: entry.displayName,
            shortName: entry.shortName,
            theme: .mod(named: entry.theme),
            mod: ModSpec(
                baseGameID: .zeroHour,
                dirName: entry.dirName,
                packageVersion: entry.packageVersion,
                downloadURLs: entry.urls.compactMap(URL.init(string:)),
                downloadSizeMB: entry.downloadSizeMB,
                diskSizeMB: entry.diskSizeMB,
                markers: [ModSpec.configFileName] + entry.markers
            )
        )
    }
}

private extension ModCatalogEntry {
    static let downloadHost = "github.com"

    var isAcceptable: Bool {
        guard !id.isEmpty, baseGame == GameID.zeroHour.rawValue, packageVersion >= 1 else {
            return false
        }

        guard Self.isFolderName(dirName) else {
            return false
        }

        guard !urls.isEmpty, urls.allSatisfy(Self.isAllowedDownload) else {
            return false
        }

        return markers.allSatisfy(Self.isRelativePath)
    }

    static func isFolderName(_ name: String) -> Bool {
        !name.isEmpty && name != "." && name != ".." && !name.contains("/")
    }

    static func isRelativePath(_ path: String) -> Bool {
        !path.isEmpty && !path.hasPrefix("/") && !path.split(separator: "/").contains("..")
    }

    static func isAllowedDownload(_ string: String) -> Bool {
        guard let url = URL(string: string) else { return false }
        return url.scheme == "https" && url.host == downloadHost
    }
}
