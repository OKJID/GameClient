import Foundation

enum ModRequestOutcome {
    case sent
    case throttled
    case failed
}

enum ModRequestSender {
    private static let endpoint = URL(string: "https://general-online-zh.web.app/api/mod-request")!
    private static let requestTimeout: TimeInterval = 15
    private static let throttledStatus = 429

    private struct Payload: Encodable {
        let name: String
        let link: String
        let launcherVersion: String
        let language: String
        let osVersion: String
    }

    private static let launcherVersion: String = {
        let info = Bundle.main.infoDictionary
        let version = info?["GOLauncherVersion"] as? String ?? "unknown"
        let build = info?["GOLauncherBuild"] as? String ?? "unknown"
        return "\(version) (build \(build))"
    }()

    static func send(name: String, link: String, completion: @escaping (ModRequestOutcome) -> Void) {
        let payload = Payload(
            name: name,
            link: link,
            launcherVersion: launcherVersion,
            language: L10n.current,
            osVersion: ProcessInfo.processInfo.operatingSystemVersionString
        )

        var request = URLRequest(url: endpoint, timeoutInterval: requestTimeout)
        request.httpMethod = "POST"
        request.setValue("application/json", forHTTPHeaderField: "Content-Type")
        request.httpBody = try? JSONEncoder().encode(payload)

        URLSession.shared.dataTask(with: request) { _, response, error in
            let status = (response as? HTTPURLResponse)?.statusCode
            let outcome = resolveOutcome(status: status, error: error)
            DispatchQueue.main.async { completion(outcome) }
        }.resume()
    }

    private static func resolveOutcome(status: Int?, error: Error?) -> ModRequestOutcome {
        guard error == nil, let status else { return .failed }

        if status == throttledStatus {
            return .throttled
        }

        return (200..<300).contains(status) ? .sent : .failed
    }
}
