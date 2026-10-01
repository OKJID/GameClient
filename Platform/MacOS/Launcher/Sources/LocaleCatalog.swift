import Foundation
import Combine

struct LocalePack: Equatable {
    let id: String
    let packageVersion: Int
    let downloadURLs: [URL]
    let downloadSizeMB: Int
    let diskSizeMB: Int
    let markers: [String]
    let hasZeroHourVoices: Bool
    let hasGeneralsVoices: Bool

    var partCount: Int { downloadURLs.count }

    func hasVoices(for profile: GameProfile) -> Bool {
        profile.baseProfile.id == GameID.generals ? hasGeneralsVoices : hasZeroHourVoices || hasGeneralsVoices
    }
}

private struct LocaleCatalogEntry: Decodable {
    struct Voices: Decodable {
        let zeroHour: Bool
        let generals: Bool
    }

    let id: String
    let packageVersion: Int
    let urls: [String]
    let downloadSizeMB: Int
    let diskSizeMB: Int
    let markers: [String]
    let voices: Voices
}

private struct LocaleCatalogPayload: Decodable {
    let version: Int
    let locales: [LenientLocaleEntry]
}

private struct LenientLocaleEntry: Decodable {
    let entry: LocaleCatalogEntry?

    init(from decoder: Decoder) throws {
        entry = try? LocaleCatalogEntry(from: decoder)
    }
}

final class LocaleCatalog: ObservableObject, ApiResource {
    static let shared = LocaleCatalog()

    private static let schemaVersion = 1

    let resourceName = "locales.json"

    @Published private(set) var packs: [String: LocalePack] = [:]

    private init() {}

    func reload() throws {
        let payload = try ApiCache.load(resourceName, decode: Self.decode)
        let entries = payload?.locales.compactMap(\.entry) ?? []
        packs = Dictionary(
            entries.filter(\.isAcceptable).map { ($0.id, Self.pack(from: $0)) },
            uniquingKeysWith: { first, _ in first }
        )
    }

    func pack(for language: String) -> LocalePack? {
        packs[language.lowercased()]
    }

    private static func decode(_ data: Data) -> LocaleCatalogPayload? {
        guard let payload = try? JSONDecoder().decode(LocaleCatalogPayload.self, from: data),
              payload.version == schemaVersion else {
            return nil
        }

        return payload
    }

    private static func pack(from entry: LocaleCatalogEntry) -> LocalePack {
        LocalePack(
            id: entry.id.lowercased(),
            packageVersion: entry.packageVersion,
            downloadURLs: entry.urls.compactMap(URL.init(string:)),
            downloadSizeMB: entry.downloadSizeMB,
            diskSizeMB: entry.diskSizeMB,
            markers: entry.markers,
            hasZeroHourVoices: entry.voices.zeroHour,
            hasGeneralsVoices: entry.voices.generals
        )
    }
}

private extension LocaleCatalogEntry {
    static let downloadHost = "github.com"

    var isAcceptable: Bool {
        guard !id.isEmpty, id.lowercased() != SettingsDefaults.gameLanguage, packageVersion >= 1 else {
            return false
        }

        guard !urls.isEmpty, urls.allSatisfy(Self.isAllowedDownload) else {
            return false
        }

        return !markers.isEmpty && markers.allSatisfy(LocaleLayout.isPackPath)
    }

    static func isAllowedDownload(_ string: String) -> Bool {
        guard let url = URL(string: string) else { return false }
        return url.scheme == "https" && url.host == downloadHost
    }
}
