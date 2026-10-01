import Foundation

struct LocaleManifest: Codable {
    let id: String
    let packageVersion: Int
    let files: [String]

    static func read(language: String, in directory: URL) -> LocaleManifest? {
        let url = LocaleLayout.manifestURL(language: language, in: directory)
        guard let data = try? Data(contentsOf: url) else { return nil }
        return try? JSONDecoder().decode(LocaleManifest.self, from: data)
    }

    func write(in directory: URL) throws {
        let encoder = JSONEncoder()
        encoder.outputFormatting = [.prettyPrinted, .sortedKeys]
        try encoder.encode(self).write(to: LocaleLayout.manifestURL(language: id, in: directory), options: .atomic)
    }
}

enum LocaleLayout {
    static let archivePrefix = "golocale"
    private static let manifestPrefix = "GOLocale-"
    private static let manifestExtension = "json"

    static func isPackPath(_ path: String) -> Bool {
        let parts = path.split(separator: "/").map(String.init)
        guard parts.count >= 2, !path.hasPrefix("/"), !parts.contains("..") else {
            return false
        }

        return GameProfile.baseGames.contains { $0.patchAssetsDirName == parts[0] }
    }

    static func isLocaleArchive(_ name: String) -> Bool {
        name.lowercased().hasPrefix(archivePrefix)
    }

    static func manifestURL(language: String, in directory: URL) -> URL {
        directory.appendingPathComponent("\(manifestPrefix)\(language).\(manifestExtension)")
    }

    static func installedURL(for packPath: String, targets: [PatchTarget]) -> URL? {
        let parts = packPath.split(separator: "/").map(String.init)
        guard let folder = parts.first,
              let target = targets.first(where: { $0.profile.patchAssetsDirName == folder }) else {
            return nil
        }

        return parts.dropFirst().reduce(target.directory) { $0.appendingPathComponent($1) }
    }

    static func hasNativeVoices(language: String, profile: GameProfile, at directory: URL) -> Bool {
        guard let entries = try? FileManager.default.contentsOfDirectory(atPath: directory.path) else {
            return false
        }

        let names = Set(entries.map { $0.lowercased() })
        return nativeVoiceArchives(language: language, profile: profile).contains { names.contains($0) }
    }

    private static func nativeVoiceArchives(language: String, profile: GameProfile) -> [String] {
        let name = language.lowercased()
        guard profile.baseProfile.id == GameID.generals else {
            return ["speech\(name)zh.big"]
        }

        return ["speech\(name).big", "speech\(name)2.big"]
    }
}
