import Foundation
import Combine

struct GuideText: Decodable, Equatable {
    let title: String
    let summary: String?
    let body: String
}

struct GuideSection: Decodable, Equatable, Identifiable {
    let id: String
    let titles: [String: String]

    func title(for language: String) -> String {
        titles[language] ?? titles["en"] ?? id
    }
}

struct GuideArticle: Decodable, Equatable, Identifiable {
    let id: String
    let section: String
    let minBuild: Int?
    let locales: [String: GuideText]

    func text(for language: String) -> GuideText? {
        locales[language] ?? locales["en"]
    }

    func isVisible(build: Int) -> Bool {
        guard let minBuild else { return true }
        return build >= minBuild
    }

    func matches(_ query: String, language: String) -> Bool {
        guard let text = text(for: language) else { return false }

        let searchable = [text.title, text.summary ?? "", text.body]
            .joined(separator: " ")
            .replacingOccurrences(of: "<[^>]+>", with: " ", options: .regularExpression)

        return query
            .split(whereSeparator: \.isWhitespace)
            .allSatisfy { searchable.localizedStandardContains($0) }
    }
}

struct GuideHint: Decodable, Equatable {
    let anchor: String
    let article: String
    let locales: [String: String]

    func text(for language: String) -> String? {
        locales[language] ?? locales["en"]
    }
}

private struct GuidePayload: Decodable {
    let version: Int
    let sections: [GuideSection]
    let articles: [Lenient<GuideArticle>]
    let hints: [Lenient<GuideHint>]?
}

private struct Lenient<Value: Decodable>: Decodable {
    let value: Value?

    init(from decoder: Decoder) throws {
        value = try? Value(from: decoder)
    }
}

final class GuideCatalog: ObservableObject, ApiResource {
    static let shared = GuideCatalog()

    private static let schemaVersion = 1
    private static let requiredLanguage = "en"

    let resourceName = "guide.json"
    let isRequired = false

    @Published private(set) var sections: [GuideSection] = []
    @Published private(set) var articles: [GuideArticle] = []
    @Published private(set) var hints: [GuideHint] = []

    private init() {}

    func reload() throws {
        let payload = try ApiCache.load(resourceName, decode: Self.decode)
        let accepted = Self.accepted(payload?.articles.compactMap(\.value) ?? [])
        let usedSections = Set(accepted.map(\.section))
        let visibleSections = (payload?.sections ?? []).filter { usedSections.contains($0.id) }
        let linkedHints = Self.accepted(payload?.hints?.compactMap(\.value) ?? [], linkedTo: accepted)
        guard accepted != articles || visibleSections != sections || linkedHints != hints else { return }

        sections = visibleSections
        articles = accepted
        hints = linkedHints
    }

    func article(id: String) -> GuideArticle? {
        articles.first { $0.id == id }
    }

    func articles(in section: GuideSection) -> [GuideArticle] {
        articles.filter { $0.section == section.id }
    }

    func hint(anchor: String) -> GuideHint? {
        hints.first { $0.anchor == anchor }
    }

    private static func decode(_ data: Data) -> GuidePayload? {
        guard let payload = try? JSONDecoder().decode(GuidePayload.self, from: data),
              payload.version == schemaVersion else {
            return nil
        }

        return payload
    }

    private static func accepted(_ raw: [GuideArticle]) -> [GuideArticle] {
        let build = UpdateChecker.currentBuild

        return raw
            .filter { $0.isVisible(build: build) }
            .filter { $0.locales[requiredLanguage] != nil }
    }

    private static func accepted(_ raw: [GuideHint], linkedTo articles: [GuideArticle]) -> [GuideHint] {
        let articleIDs = Set(articles.map(\.id))

        return raw
            .filter { articleIDs.contains($0.article) }
            .filter { $0.locales[requiredLanguage] != nil }
    }
}
