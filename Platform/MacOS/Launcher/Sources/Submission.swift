import Foundation

enum SubmissionOutcome {
    case sent
    case throttled
    case failed

    private static let throttledStatus = 429

    init(status: Int?, error: Error?) {
        guard error == nil, let status else {
            self = .failed
            return
        }

        if status == Self.throttledStatus {
            self = .throttled
            return
        }

        self = (200..<300).contains(status) ? .sent : .failed
    }
}

enum Submission {
    static let apiBase = URL(string: "https://general-online-zh.web.app/api/")!

    static let launcherVersion: String = {
        let info = Bundle.main.infoDictionary
        let version = info?["GOLauncherVersion"] as? String ?? "unknown"
        let build = info?["GOLauncherBuild"] as? String ?? "unknown"
        let engine = info?["CFBundleShortVersionString"] as? String ?? "unknown"
        return "\(version) (build \(build)), engine \(engine)"
    }()

    static let osVersion = ProcessInfo.processInfo.operatingSystemVersionString

    static let machine: String = {
        let model = sysctlString("hw.model") ?? "unknown"
        let chip = sysctlString("machdep.cpu.brand_string") ?? "unknown"
        let memoryGB = ProcessInfo.processInfo.physicalMemory / (1024 * 1024 * 1024)
        return "\(model) · \(chip) · \(memoryGB) GB"
    }()

    static func log(endpoint: URL, status: Int?, error: Error?, outcome: SubmissionOutcome) {
        let statusText = status.map(String.init) ?? "none"
        let errorText = error?.localizedDescription ?? "none"
        print("[Submission] \(endpoint.absoluteString) status=\(statusText) error=\(errorText) outcome=\(outcome)")
        fflush(stdout)
    }

    private static func sysctlString(_ name: String) -> String? {
        var size = 0
        guard sysctlbyname(name, nil, &size, nil, 0) == 0, size > 0 else { return nil }

        var buffer = [CChar](repeating: 0, count: size)
        guard sysctlbyname(name, &buffer, &size, nil, 0) == 0 else { return nil }

        return String(cString: buffer)
    }
}
