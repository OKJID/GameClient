import Foundation

struct PendingCrash {
    let profile: GameProfile
    let crashedAt: TimeInterval

    var id: String { "\(profile.id.rawValue):\(crashedAt)" }
}

enum CrashReports {
    private static let reportedKeyPrefix = "CRASH_REPORTED_AT_"
    private static let promptedKeyPrefix = "CRASH_PROMPTED_AT_"

    static func pending() -> PendingCrash? {
        GameProfile.baseGames.lazy.compactMap(pendingCrash(of:)).first
    }

    static func wasPrompted(_ crash: PendingCrash) -> Bool {
        storedTime(promptedKeyPrefix, crash.profile) >= crash.crashedAt
    }

    static func markPrompted(_ crash: PendingCrash) {
        storeTime(crash.crashedAt, promptedKeyPrefix, crash.profile)
    }

    static func markReported(_ crash: PendingCrash) {
        storeTime(crash.crashedAt, reportedKeyPrefix, crash.profile)
    }

    private static func pendingCrash(of profile: GameProfile) -> PendingCrash? {
        guard let crashedAt = latestCrashTime(in: profile.userDataDirURL) else { return nil }
        guard crashedAt > storedTime(reportedKeyPrefix, profile) else { return nil }

        return PendingCrash(profile: profile, crashedAt: crashedAt)
    }

    private static func latestCrashTime(in base: URL) -> TimeInterval? {
        guard let newestReport = LogsSharer.crashReportNames.first else { return nil }

        let path = base.appendingPathComponent(newestReport).path
        let attributes = try? FileManager.default.attributesOfItem(atPath: path)
        return (attributes?[.modificationDate] as? Date)?.timeIntervalSince1970
    }

    private static func storedTime(_ prefix: String, _ profile: GameProfile) -> TimeInterval {
        UserDefaults.standard.double(forKey: prefix + profile.id.rawValue)
    }

    private static func storeTime(_ time: TimeInterval, _ prefix: String, _ profile: GameProfile) {
        UserDefaults.standard.set(time, forKey: prefix + profile.id.rawValue)
    }
}
