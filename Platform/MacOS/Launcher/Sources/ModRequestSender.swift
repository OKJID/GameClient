import Foundation

enum ModAuthorRequestKind: String, CaseIterable, Identifiable {
    case change
    case removal

    var id: String { rawValue }
}

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

    private struct AuthorPayload: Encodable {
        let kind: String
        let name: String
        let modId: String
        let contact: String
        let message: String
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

        post(try? JSONEncoder().encode(payload), completion: completion)
    }

    static func sendAuthorRequest(
        kind: ModAuthorRequestKind,
        profile: GameProfile,
        contact: String,
        message: String,
        completion: @escaping (SubmissionOutcome) -> Void
    ) {
        let payload = AuthorPayload(
            kind: kind.rawValue,
            name: profile.displayName,
            modId: profile.id.rawValue,
            contact: contact,
            message: message,
            launcherVersion: Submission.launcherVersion,
            language: L10n.current,
            osVersion: Submission.osVersion
        )

        post(try? JSONEncoder().encode(payload), completion: completion)
    }

    private static func post(_ body: Data?, completion: @escaping (SubmissionOutcome) -> Void) {
        var request = URLRequest(url: endpoint, timeoutInterval: requestTimeout)
        request.httpMethod = "POST"
        request.setValue("application/json", forHTTPHeaderField: "Content-Type")
        request.httpBody = body

        URLSession.shared.dataTask(with: request) { _, response, error in
            let status = (response as? HTTPURLResponse)?.statusCode
            let outcome = SubmissionOutcome(status: status, error: error)
            Submission.log(endpoint: endpoint, status: status, error: error, outcome: outcome)
            DispatchQueue.main.async { completion(outcome) }
        }.resume()
    }
}
