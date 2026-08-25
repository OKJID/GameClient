import Foundation

struct PatchVersions {
    static let current = "3"
    static let markerFileName = "MacPatch.version"

    static func isCurrent(at directory: URL) -> Bool {
        installedVersion(at: directory) == current
    }

    static func installedVersion(at directory: URL) -> String? {
        let url = directory.appendingPathComponent(markerFileName)
        guard let raw = try? String(contentsOf: url, encoding: .utf8) else {
            return nil
        }

        let version = raw.trimmingCharacters(in: .whitespacesAndNewlines)
        return version.isEmpty ? nil : version
    }

    static func markCurrent(at directory: URL) throws {
        let url = directory.appendingPathComponent(markerFileName)
        try current.write(to: url, atomically: true, encoding: .utf8)
    }
}
