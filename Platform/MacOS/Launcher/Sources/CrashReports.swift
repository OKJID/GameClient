import Foundation

enum CrashSource: CaseIterable {
    case game
    case worldBuilder

    var fileNamePrefix: String {
        switch self {
        case .game: return ""
        case .worldBuilder: return "wb_"
        }
    }

    var storageSuffix: String {
        switch self {
        case .game: return ""
        case .worldBuilder: return "_WORLDBUILDER"
        }
    }

    var nameSuffix: String {
        switch self {
        case .game: return ""
        case .worldBuilder: return " WorldBuilder"
        }
    }
}

struct PendingCrash {
    let profile: GameProfile
    let source: CrashSource
    let crashedAt: TimeInterval

    var id: String { "\(storageKey):\(crashedAt)" }
    var storageKey: String { profile.id.rawValue + source.storageSuffix }
    var gameName: String { profile.displayName + source.nameSuffix }

    var reportURLs: [URL] {
        LogsSharer.existingCrashReports(in: profile.userDataDirURL, prefix: source.fileNamePrefix)
    }
}

enum CrashReports {
    private static let reportedKeyPrefix = "CRASH_REPORTED_AT_"
    private static let promptedKeyPrefix = "CRASH_PROMPTED_AT_"

    static func pending() -> PendingCrash? {
        for profile in GameProfile.baseGames {
            for source in CrashSource.allCases {
                if let crash = pendingCrash(of: profile, source: source) {
                    return crash
                }
            }
        }

        return nil
    }

    static func wasPrompted(_ crash: PendingCrash) -> Bool {
        storedTime(promptedKeyPrefix, crash.storageKey) >= crash.crashedAt
    }

    static func markPrompted(_ crash: PendingCrash) {
        storeTime(crash.crashedAt, promptedKeyPrefix, crash.storageKey)
    }

    static func markReported(_ crash: PendingCrash) {
        storeTime(crash.crashedAt, reportedKeyPrefix, crash.storageKey)
    }

    private static func pendingCrash(of profile: GameProfile, source: CrashSource) -> PendingCrash? {
        guard let crashedAt = latestCrashTime(in: profile.userDataDirURL, prefix: source.fileNamePrefix) else { return nil }

        let crash = PendingCrash(profile: profile, source: source, crashedAt: crashedAt)
        guard crashedAt > storedTime(reportedKeyPrefix, crash.storageKey) else { return nil }

        return crash
    }

    private static func latestCrashTime(in base: URL, prefix: String) -> TimeInterval? {
        guard let newestReport = LogsSharer.crashReportNames.first else { return nil }

        let path = base.appendingPathComponent(prefix + newestReport).path
        let attributes = try? FileManager.default.attributesOfItem(atPath: path)
        return (attributes?[.modificationDate] as? Date)?.timeIntervalSince1970
    }

    private static func storedTime(_ prefix: String, _ key: String) -> TimeInterval {
        UserDefaults.standard.double(forKey: prefix + key)
    }

    private static func storeTime(_ time: TimeInterval, _ prefix: String, _ key: String) {
        UserDefaults.standard.set(time, forKey: prefix + key)
    }
}
