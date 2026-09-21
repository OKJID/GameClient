import Foundation
import Combine

enum CommunityLinks {
    static let telegram = "https://t.me/GeneralsOnlineMacOS"
    static let discord = "https://discord.gg/Mm3yjnv6V"
}

struct Announcement: Codable, Equatable, Identifiable {
    let id: String
    let durationMs: Int?
    let minBuild: Int?
    let body: String?
    let locales: [String: String]?

    var duration: TimeInterval {
        guard let durationMs, durationMs >= 2000 else { return 8 }
        return TimeInterval(min(durationMs, 120_000)) / 1000
    }

    func html(for language: String) -> String? {
        if let localized = locales?[language], !localized.isEmpty {
            return localized
        }

        if let english = locales?["en"], !english.isEmpty {
            return english
        }

        guard let body, !body.isEmpty else { return nil }
        return body
    }

    func isVisible(build: Int) -> Bool {
        guard let minBuild else { return true }
        return build >= minBuild
    }
}

private struct AnnouncementsPayload: Codable {
    let version: Int?
    let items: [Announcement]
}

final class AnnouncementsFeed: ObservableObject, ApiResource {
    private static let schemaVersion = 1
    private static let maxItems = 12
    private static let maxBodyLength = 4000

    let resourceName = "announcements.json"

    @Published private(set) var items: [Announcement] = []

    func reload() throws {
        let payload = try ApiCache.load(resourceName, decode: Self.decode)
        let accepted = Self.accepted(payload?.items ?? [])
        guard accepted != items else { return }

        items = accepted
    }

    private static func decode(_ data: Data) -> AnnouncementsPayload? {
        guard let payload = try? JSONDecoder().decode(AnnouncementsPayload.self, from: data),
              payload.version == schemaVersion else {
            return nil
        }

        return payload
    }

    private static func accepted(_ raw: [Announcement]) -> [Announcement] {
        let build = UpdateChecker.currentBuild

        return raw
            .filter { $0.isVisible(build: build) }
            .filter { $0.html(for: L10n.current) != nil }
            .map(truncated)
            .prefix(maxItems)
            .map { $0 }
    }

    private static func truncated(_ item: Announcement) -> Announcement {
        let limited = item.locales?.mapValues { String($0.prefix(maxBodyLength)) }

        return Announcement(
            id: item.id,
            durationMs: item.durationMs,
            minBuild: item.minBuild,
            body: item.body.map { String($0.prefix(maxBodyLength)) },
            locales: limited
        )
    }
}
