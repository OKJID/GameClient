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

class UpdateChecker: ObservableObject {
    static var currentVersion: String {
        Bundle.main.infoDictionary?["GOLauncherVersion"] as? String ?? "0.0.0"
    }
    static var currentBuild: Int {
        Int(Bundle.main.infoDictionary?["GOLauncherBuild"] as? String ?? "0") ?? 0
    }

    private static let updateURL = URL(string: "https://general-online-zh.web.app/api/update.json")!
    private static let checkInterval: TimeInterval = 5 * 60

    @Published var availableUpdate: AppUpdate? = nil
    @Published var isChecking: Bool = false
    @Published var lastCheckDate: Date? = nil

    private var timer: Timer?

    func startPeriodicChecks() {
        checkForUpdate()

        timer = Timer.scheduledTimer(withTimeInterval: Self.checkInterval, repeats: true) { [weak self] _ in
            self?.checkForUpdate()
        }
    }

    func stopPeriodicChecks() {
        timer?.invalidate()
        timer = nil
    }

    func checkForUpdate() {
        guard !isChecking else { return }
        isChecking = true

        var request = URLRequest(url: Self.updateURL)
        request.cachePolicy = .reloadIgnoringLocalAndRemoteCacheData
        request.timeoutInterval = 10

        URLSession.shared.dataTask(with: request) { [weak self] data, response, error in
            DispatchQueue.main.async {
                guard let self else { return }
                self.isChecking = false
                self.lastCheckDate = Date()

                guard let data, error == nil else { return }
                guard let update = try? JSONDecoder().decode(AppUpdate.self, from: data) else { return }

                if self.isNewerVersion(update.version, than: Self.currentVersion)
                    || update.build > Self.currentBuild {
                    self.availableUpdate = update
                }
            }
        }.resume()
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
