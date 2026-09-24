import Foundation
import UniformTypeIdentifiers

struct SupportRequest {
    enum Kind: String {
        case support
        case crash
    }

    let kind: Kind
    let message: String
    let telegram: String
    let discord: String
    let email: String
    let game: String
    let logFiles: [URL]
    let logsArchivePrefix: String
    let replay: URL?
    let attachment: URL?

    static func crashArchivePrefix(of profile: GameProfile) -> String {
        "\(profile.logsArchivePrefix)_Crash_"
    }

    static func crashReport(_ crash: PendingCrash) -> SupportRequest {
        SupportRequest(
            kind: .crash,
            message: "",
            telegram: "",
            discord: "",
            email: "",
            game: crash.profile.displayName,
            logFiles: LogsSharer.existingCrashReports(in: crash.profile.userDataDirURL),
            logsArchivePrefix: crashArchivePrefix(of: crash.profile),
            replay: nil,
            attachment: nil
        )
    }
}

enum SupportSender {
    static let attachmentLimitMB = 10

    private static let endpoint = Submission.apiBase.appendingPathComponent("support")
    private static let requestTimeout: TimeInterval = 120

    private struct FilePart {
        let fieldName: String
        let url: URL
    }

    static func send(_ request: SupportRequest, completion: @escaping (SubmissionOutcome) -> Void) {
        DispatchQueue.global(qos: .userInitiated).async {
            let boundary = "Boundary-\(UUID().uuidString)"

            var urlRequest = URLRequest(url: endpoint, timeoutInterval: requestTimeout)
            urlRequest.httpMethod = "POST"
            urlRequest.setValue("multipart/form-data; boundary=\(boundary)", forHTTPHeaderField: "Content-Type")
            urlRequest.httpBody = body(of: request, boundary: boundary)

            URLSession.shared.dataTask(with: urlRequest) { _, response, error in
                let status = (response as? HTTPURLResponse)?.statusCode
                let outcome = SubmissionOutcome(status: status, error: error)
                Submission.log(endpoint: endpoint, status: status, error: error, outcome: outcome)
                DispatchQueue.main.async { completion(outcome) }
            }.resume()
        }
    }

    private static func fields(of request: SupportRequest) -> [(String, String)] {
        [
            ("kind", request.kind.rawValue),
            ("message", request.message),
            ("telegram", request.telegram),
            ("discord", request.discord),
            ("email", request.email),
            ("game", request.game),
            ("launcherVersion", Submission.launcherVersion),
            ("language", L10n.current),
            ("osVersion", Submission.osVersion),
            ("machine", Submission.machine)
        ]
    }

    private static func fileParts(of request: SupportRequest) -> [FilePart] {
        var parts: [FilePart] = []

        if !request.logFiles.isEmpty, let archive = LogsSharer.makeZip(of: request.logFiles, prefix: request.logsArchivePrefix) {
            parts.append(FilePart(fieldName: "logs", url: archive))
        }

        if let replay = request.replay {
            parts.append(FilePart(fieldName: "replay", url: replay))
        }

        if let attachment = request.attachment {
            parts.append(FilePart(fieldName: "attachment", url: attachment))
        }

        return parts
    }

    private static func body(of request: SupportRequest, boundary: String) -> Data {
        var body = Data()

        for (name, value) in fields(of: request) {
            body.append("--\(boundary)\r\n")
            body.append("Content-Disposition: form-data; name=\"\(name)\"\r\n\r\n")
            body.append("\(value)\r\n")
        }

        for part in fileParts(of: request) {
            guard let data = try? Data(contentsOf: part.url) else { continue }

            body.append("--\(boundary)\r\n")
            body.append("Content-Disposition: form-data; name=\"\(part.fieldName)\"; filename=\"\(quotedFileName(of: part.url))\"\r\n")
            body.append("Content-Type: \(mimeType(of: part.url))\r\n\r\n")
            body.append(data)
            body.append("\r\n")
        }

        body.append("--\(boundary)--\r\n")
        return body
    }

    private static func quotedFileName(of url: URL) -> String {
        url.lastPathComponent
            .replacingOccurrences(of: "\"", with: "'")
            .replacingOccurrences(of: "\r", with: " ")
            .replacingOccurrences(of: "\n", with: " ")
    }

    private static func mimeType(of url: URL) -> String {
        UTType(filenameExtension: url.pathExtension)?.preferredMIMEType ?? "application/octet-stream"
    }
}

private extension Data {
    mutating func append(_ text: String) {
        append(Data(text.utf8))
    }
}
