import AppKit
import Combine

enum DonationIcon: Equatable {
    case symbol(String)
    case svg(source: String, image: NSImage)

    static func == (lhs: DonationIcon, rhs: DonationIcon) -> Bool {
        switch (lhs, rhs) {
        case let (.symbol(left), .symbol(right)):
            return left == right
        case let (.svg(left, _), .svg(right, _)):
            return left == right
        default:
            return false
        }
    }
}

struct DonationMethod: Identifiable, Equatable {
    let id: String
    let url: URL
    let icon: DonationIcon
    let titles: [String: String]
    let subtitles: [String: String]

    var title: String { Donations.localized(titles) ?? id }
    var subtitle: String? { Donations.localized(subtitles) }
}

private struct RawDonationMethod: Decodable {
    let id: String
    let url: String
    let icon: String?
    let iconSvg: String?
    let title: [String: String]
    let subtitle: [String: String]?
}

private struct LenientDonationMethod: Decodable {
    let method: RawDonationMethod?

    init(from decoder: Decoder) throws {
        method = try? RawDonationMethod(from: decoder)
    }
}

private struct DonationsPayload: Decodable {
    let version: Int
    let items: [LenientDonationMethod]
}

enum Donations {
    static func localized(_ texts: [String: String]) -> String? {
        if let text = texts[L10n.current], !text.isEmpty {
            return text
        }

        guard let english = texts["en"], !english.isEmpty else { return nil }
        return english
    }
}

final class DonationsCatalog: ObservableObject, ApiResource {
    static let shared = DonationsCatalog()

    private static let schemaVersion = 1
    private static let maxItems = 6
    private static let fallbackSymbol = "heart.fill"
    private static let maxSvgLength = 8 * 1024

    let resourceName = "donations.json"

    @Published private(set) var methods: [DonationMethod] = []

    private init() {}

    func reload() throws {
        let payload = try ApiCache.load(resourceName, decode: Self.decode)
        let accepted = Self.accepted(payload?.items.compactMap(\.method) ?? [])

        guard accepted != methods else { return }
        methods = accepted
    }

    private static func decode(_ data: Data) -> DonationsPayload? {
        guard let payload = try? JSONDecoder().decode(DonationsPayload.self, from: data),
              payload.version == schemaVersion else {
            return nil
        }

        return payload
    }

    private static func accepted(_ raw: [RawDonationMethod]) -> [DonationMethod] {
        var usedIDs = Set<String>()

        return raw
            .compactMap(method)
            .filter { usedIDs.insert($0.id).inserted }
            .prefix(maxItems)
            .map { $0 }
    }

    private static func method(from raw: RawDonationMethod) -> DonationMethod? {
        guard !raw.id.isEmpty, let url = secureURL(raw.url) else {
            return nil
        }

        guard let englishTitle = raw.title["en"], !englishTitle.isEmpty else {
            return nil
        }

        return DonationMethod(
            id: raw.id,
            url: url,
            icon: icon(from: raw),
            titles: raw.title,
            subtitles: raw.subtitle ?? [:]
        )
    }

    private static func icon(from raw: RawDonationMethod) -> DonationIcon {
        if let source = raw.iconSvg, let image = svgImage(source) {
            return .svg(source: source, image: image)
        }

        return .symbol(raw.icon.flatMap(systemSymbol) ?? fallbackSymbol)
    }

    private static func svgImage(_ source: String) -> NSImage? {
        guard source.utf8.count <= maxSvgLength,
              let image = NSImage(data: Data(source.utf8)),
              image.size.width > 0, image.size.height > 0 else {
            return nil
        }

        return image
    }

    private static func secureURL(_ string: String) -> URL? {
        guard let url = URL(string: string), url.scheme?.lowercased() == "https", url.host != nil else {
            return nil
        }

        return url
    }

    private static func systemSymbol(_ name: String) -> String? {
        NSImage(systemSymbolName: name, accessibilityDescription: nil) == nil ? nil : name
    }
}
