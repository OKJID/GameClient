import Foundation
import Combine

struct AppUpdate: Codable {
    let version: String
    let build: Int
    let downloadURL: String
    let releaseNotes: String
    let releaseDate: String
    let minOSVersion: String
}

class UpdateChecker: ObservableObject, ApiResource {
    static var currentVersion: String {
        Bundle.main.infoDictionary?["GOLauncherVersion"] as? String ?? "0.0.0"
    }
    static var currentBuild: Int {
        Int(Bundle.main.infoDictionary?["GOLauncherBuild"] as? String ?? "0") ?? 0
    }
    // Empty outside an assembled bundle: assemble_distribution.sh is what stamps the keys,
    // and a dev run showing "v0.0.0 (0)" reads as a shipped version.
    static var currentVersionLabel: String {
        guard let version = Bundle.main.infoDictionary?["GOLauncherVersion"] as? String else { return "" }

        return "v\(version) (\(currentBuild))"
    }

    let resourceName = "update.json"

    @Published var availableUpdate: AppUpdate? = nil

    func reload() throws {
        let update = try ApiCache.load(resourceName) { try? JSONDecoder().decode(AppUpdate.self, from: $0) }
        offer(update.flatMap { isOffered($0) ? $0 : nil })
    }

    private func offer(_ update: AppUpdate?) {
        guard update?.version != availableUpdate?.version else { return }

        availableUpdate = update
        guard let update else { return }
        Analytics.logUpdateOffered(version: update.version)
    }

    private func isOffered(_ update: AppUpdate) -> Bool {
        isNewerVersion(update.version, than: Self.currentVersion) || update.build > Self.currentBuild
    }

    private func isNewerVersion(_ remote: String, than local: String) -> Bool {
        let r = remote.split(separator: ".").compactMap { Int($0) }
        let l = local.split(separator: ".").compactMap { Int($0) }

        for i in 0..<max(r.count, l.count) {
            let rv = i < r.count ? r[i] : 0
            let lv = i < l.count ? l[i] : 0
            if rv > lv { return true }
            if rv < lv { return false }
        }
        return false
    }
}
