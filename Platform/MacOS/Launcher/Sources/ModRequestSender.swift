import Foundation

enum ModRequestSender {
    private static let endpoint = Submission.apiBase.appendingPathComponent("mod-request")
    private static let requestTimeout: TimeInterval = 15

    private struct Payload: Encodable {
        let name: String
        let link: String
        let launcherVersion: String
        let language: String
        let osVersion: String
    }

    static func send(name: String, link: String, completion: @escaping (SubmissionOutcome) -> Void) {
        let payload = Payload(
            name: name,
            link: link,
            launcherVersion: Submission.launcherVersion,
            language: L10n.current,
            osVersion: Submission.osVersion
        )

        var request = URLRequest(url: endpoint, timeoutInterval: requestTimeout)
        request.httpMethod = "POST"
        request.setValue("application/json", forHTTPHeaderField: "Content-Type")
        request.httpBody = try? JSONEncoder().encode(payload)

        URLSession.shared.dataTask(with: request) { _, response, error in
            let status = (response as? HTTPURLResponse)?.statusCode
            let outcome = SubmissionOutcome(status: status, error: error)
            Submission.log(endpoint: endpoint, status: status, error: error, outcome: outcome)
            DispatchQueue.main.async { completion(outcome) }
        }.resume()
    }
}
