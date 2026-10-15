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
    var storageKey: String { profile.baseProfile.id.rawValue + source.storageSuffix }
    var gameName: String { profile.displayName + source.nameSuffix }

    var reportURLs: [URL] {
        LogsSharer.existingCrashReports(in: profile.userDataDirURL, prefix: source.fileNamePrefix)
    }
}

enum CrashReports {
    private static let reportedKeyPrefix = "CRASH_REPORTED_AT_"
    private static let promptedKeyPrefix = "CRASH_PROMPTED_AT_"
    private static let crashArgsPrefix = "CRASH ARGS:"
    private static let modArgument = " -mod "

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

        let crashedProfile = crashedProfile(of: profile, prefix: source.fileNamePrefix)
        let crash = PendingCrash(profile: crashedProfile, source: source, crashedAt: crashedAt)
        guard crashedAt > storedTime(reportedKeyPrefix, crash.storageKey) else { return nil }

        return crash
    }

    private static func crashedProfile(of baseProfile: GameProfile, prefix: String) -> GameProfile {
        guard let modDirName = crashedModDirName(in: baseProfile.userDataDirURL, prefix: prefix) else { return baseProfile }

        let modProfile = GameProfile.mods.first { profile in
            guard let mod = profile.mod, mod.baseGameID == baseProfile.id else { return false }
            return mod.dirName.caseInsensitiveCompare(modDirName) == .orderedSame
        }

        return modProfile ?? baseProfile
    }

    private static func crashedModDirName(in base: URL, prefix: String) -> String? {
        guard let newestReport = LogsSharer.crashReportNames.first else { return nil }
        guard let data = try? Data(contentsOf: base.appendingPathComponent(prefix + newestReport)) else { return nil }

        let report = String(decoding: data, as: UTF8.self)
        guard let argsLine = report.split(whereSeparator: \.isNewline).first(where: { $0.hasPrefix(crashArgsPrefix) }) else { return nil }
        guard let modRange = argsLine.range(of: modArgument) else { return nil }

        let manifestPath = argsLine[modRange.upperBound...]
        guard let manifestRange = manifestPath.range(of: "/" + ModSpec.configFileName) else { return nil }

        let modDirectory = String(manifestPath[..<manifestRange.lowerBound])
        return URL(fileURLWithPath: modDirectory).lastPathComponent
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
