import Foundation
import Combine

enum CommunityLinks {
    static let telegram = "https://t.me/GeneralsOnlineMacOS"
    static let discord = "https://discord.gg/vzUbZP5Cj"
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

final class AnnouncementsFeed: ObservableObject {
    private static let feedURL = URL(string: "https://general-online-zh.web.app/api/announcements.json")!
    private static let refreshInterval: TimeInterval = 30 * 60
    private static let requestTimeout: TimeInterval = 10
    private static let maxItems = 12
    private static let maxBodyLength = 4000

    @Published private(set) var items: [Announcement] = []

    private var timer: Timer?

    func start() {
        items = Self.readCache()
        refresh()

        timer = Timer.scheduledTimer(withTimeInterval: Self.refreshInterval, repeats: true) { [weak self] _ in
            self?.refresh()
        }
    }

    func stop() {
        timer?.invalidate()
        timer = nil
    }

    func refresh() {
        var request = URLRequest(url: Self.feedURL)
        request.cachePolicy = .reloadIgnoringLocalAndRemoteCacheData
        request.timeoutInterval = Self.requestTimeout

        URLSession.shared.dataTask(with: request) { [weak self] data, _, error in
            guard let data, error == nil else { return }
            guard let payload = try? JSONDecoder().decode(AnnouncementsPayload.self, from: data) else { return }

            let accepted = Self.accepted(payload.items)

            DispatchQueue.main.async {
                guard let self else { return }
                guard accepted != self.items else { return }

                self.items = accepted
                Self.writeCache(accepted)
            }
        }.resume()
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

private extension AnnouncementsFeed {
    static var cacheURL: URL? {
        guard let base = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask).first else {
            return nil
        }

        let folder = base.appendingPathComponent("GeneralsOnlineLauncher", isDirectory: true)
        try? FileManager.default.createDirectory(at: folder, withIntermediateDirectories: true)

        return folder.appendingPathComponent("announcements.json")
    }

    static func readCache() -> [Announcement] {
        guard let cacheURL, let data = try? Data(contentsOf: cacheURL) else { return [] }
        guard let cached = try? JSONDecoder().decode([Announcement].self, from: data) else { return [] }

        return accepted(cached)
    }

    static func writeCache(_ items: [Announcement]) {
        guard let cacheURL, let data = try? JSONEncoder().encode(items) else { return }
        try? data.write(to: cacheURL, options: .atomic)
    }
}
