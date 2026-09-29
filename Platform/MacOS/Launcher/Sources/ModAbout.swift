import Foundation
import Combine

enum ModAttribution: String {
    case credited
    case unknown
    case approved
}

enum ModLinkKind: String {
    case moddb
    case discord
    case website
    case youtube
    case facebook
    case github
    case support

    var title: String {
        switch self {
        case .moddb: return "ModDB"
        case .discord: return "Discord"
        case .youtube: return "YouTube"
        case .facebook: return "Facebook"
        case .github: return "GitHub"
        case .website: return L10n.mod.about.website
        case .support: return L10n.mod.about.support
        }
    }

    var icon: String {
        switch self {
        case .moddb: return "puzzlepiece.extension"
        case .discord: return "bubble.left.and.bubble.right"
        case .youtube: return "play.rectangle"
        case .facebook: return "person.2"
        case .github: return "chevron.left.forwardslash.chevron.right"
        case .website: return "globe"
        case .support: return "heart"
        }
    }
}

struct ModLink: Identifiable {
    let kind: ModLinkKind?
    let label: String?
    let url: URL

    var id: String { url.absoluteString }

    var title: String {
        let kindTitle = kind?.title ?? url.host ?? url.absoluteString
        guard let label, !label.isEmpty else { return kindTitle }
        return "\(kindTitle) · \(label)"
    }

    var icon: String { kind?.icon ?? "link" }
}

struct ModInclude: Identifiable {
    let name: String
    let authors: String?
    let url: URL?

    var id: String { name }
}

struct ModAbout {
    let version: String?
    let authors: String?
    let attribution: ModAttribution
    let descriptions: [String: String]
    let includes: [ModInclude]
    let links: [ModLink]

    func description(for language: String) -> String? {
        descriptions[language] ?? descriptions["en"]
    }
}

private struct RawLink: Decodable {
    let kind: String
    let label: String?
    let url: String
}

private struct RawInclude: Decodable {
    let name: String
    let authors: String?
    let url: String?
}

private struct RawAbout: Decodable {
    let version: String?
    let authors: String?
    let attribution: String?
    let description: [String: String]?
    let includes: [RawInclude]?
    let links: [RawLink]?
}

private struct LenientAbout: Decodable {
    let about: RawAbout?

    init(from decoder: Decoder) throws {
        about = try? RawAbout(from: decoder)
    }
}

private struct ModAboutPayload: Decodable {
    let version: Int
    let mods: [String: LenientAbout]
}

final class ModAboutCatalog: ObservableObject, ApiResource {
    static let shared = ModAboutCatalog()

    private static let schemaVersion = 1
    private static let allowedSchemes: Set<String> = ["https", "http"]

    let resourceName = "mods-about.json"

    @Published private(set) var entries: [String: ModAbout] = [:]

    private init() {}

    func reload() throws {
        let payload = try ApiCache.load(resourceName, decode: Self.decode)
        entries = payload?.mods.compactMapValues { $0.about.map(Self.about) } ?? [:]
    }

    func about(for id: GameID) -> ModAbout? {
        entries[id.rawValue]
    }

    private static func decode(_ data: Data) -> ModAboutPayload? {
        guard let payload = try? JSONDecoder().decode(ModAboutPayload.self, from: data),
              payload.version == schemaVersion else {
            return nil
        }

        return payload
    }

    private static func about(from raw: RawAbout) -> ModAbout {
        ModAbout(
            version: raw.version,
            authors: raw.authors,
            attribution: raw.attribution.flatMap(ModAttribution.init(rawValue:)) ?? .credited,
            descriptions: raw.description ?? [:],
            includes: (raw.includes ?? []).map(include),
            links: (raw.links ?? []).compactMap(link)
        )
    }

    private static func include(from raw: RawInclude) -> ModInclude {
        ModInclude(name: raw.name, authors: raw.authors, url: raw.url.flatMap(safeURL))
    }

    private static func link(from raw: RawLink) -> ModLink? {
        guard let url = safeURL(raw.url) else { return nil }
        return ModLink(kind: ModLinkKind(rawValue: raw.kind), label: raw.label, url: url)
    }

    private static func safeURL(_ string: String) -> URL? {
        guard let url = URL(string: string),
              let scheme = url.scheme?.lowercased(),
              allowedSchemes.contains(scheme) else {
            return nil
        }

        return url
    }
}
