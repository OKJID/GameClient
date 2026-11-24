import Foundation

protocol OwnModArtworkRule {
    func pick(from files: [URL]) -> URL?
}

struct OwnModNamedArtworkRule: OwnModArtworkRule {
    let fileName: String

    func pick(from files: [URL]) -> URL? {
        files.first { $0.lastPathComponent.caseInsensitiveCompare(fileName) == .orderedSame }
    }
}

struct OwnModAnyImageArtworkRule: OwnModArtworkRule {
    let extensions: Set<String>

    func pick(from files: [URL]) -> URL? {
        files
            .filter { extensions.contains($0.pathExtension.lowercased()) }
            .min { $0.lastPathComponent.localizedStandardCompare($1.lastPathComponent) == .orderedAscending }
    }
}

struct OwnModArtworkLocator {
    let rules: [OwnModArtworkRule]

    static let standard = OwnModArtworkLocator(rules: [
        OwnModNamedArtworkRule(fileName: "Install_Final.bmp"),
        OwnModAnyImageArtworkRule(extensions: ["png", "jpg", "jpeg", "bmp"])
    ])

    func artworkURL(in folder: URL) -> URL? {
        let files = Self.rootFiles(of: folder)
        return rules.lazy.compactMap { $0.pick(from: files) }.first
    }

    private static func rootFiles(of folder: URL) -> [URL] {
        let entries = (try? FileManager.default.contentsOfDirectory(
            at: folder,
            includingPropertiesForKeys: [.isRegularFileKey],
            options: .skipsHiddenFiles
        )) ?? []

        return entries.filter { (try? $0.resourceValues(forKeys: [.isRegularFileKey]).isRegularFile) == true }
    }
}
