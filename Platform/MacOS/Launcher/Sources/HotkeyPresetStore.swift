import Foundation

enum HotkeyPresetStore {
    static let shippedExtension = "json"
    static let ownExtensions = ["csf", "big"]

    static func entries(in folder: URL, for profile: HotkeyDump.Profile) -> [HotkeyPresetEntry] {
        let urls = files(in: folder)
        let shipped = urls
            .filter { $0.pathExtension.lowercased() == shippedExtension }
            .compactMap { url in load(url).map { HotkeyPresetEntry(url: url, kind: .shipped($0)) } }
            .filter { entry in
                guard case .shipped(let preset) = entry.kind else { return false }
                return preset.suits(profile)
            }
        let own = urls
            .filter { ownExtensions.contains($0.pathExtension.lowercased()) }
            .map { HotkeyPresetEntry(url: $0, kind: .own) }

        return (shipped + own).sorted { $0.title.localizedCaseInsensitiveCompare($1.title) == .orderedAscending }
    }

    static func entry(named name: String, in folder: URL) -> HotkeyPresetEntry? {
        files(in: folder).lazy.compactMap { url -> HotkeyPresetEntry? in
            let extensionName = url.pathExtension.lowercased()
            if extensionName == shippedExtension {
                return load(url).map { HotkeyPresetEntry(url: url, kind: .shipped($0)) }
            }
            return ownExtensions.contains(extensionName) ? HotkeyPresetEntry(url: url, kind: .own) : nil
        }
        .first { $0.name == name }
    }

    static func load(_ url: URL) -> HotkeyPreset? {
        guard let data = try? Data(contentsOf: url) else { return nil }
        return try? JSONDecoder().decode(HotkeyPreset.self, from: data)
    }

    static func isShippedCurrent(in folder: URL, packageVersion: Int) -> Bool {
        let versions = shippedFiles(in: folder).map { load($0)?.version ?? 0 }
        return !versions.isEmpty && versions.allSatisfy { $0 >= packageVersion }
    }

    static func replaceShipped(in folder: URL, with sources: [URL]) throws {
        let fm = FileManager.default
        try fm.createDirectory(at: folder, withIntermediateDirectories: true)
        try shippedFiles(in: folder).forEach { try fm.removeItem(at: $0) }

        for source in sources {
            try fm.copyItem(at: source, to: folder.appendingPathComponent(source.lastPathComponent))
        }
    }

    static func addOwn(_ source: URL, to folder: URL) -> URL? {
        let fm = FileManager.default
        let destination = folder.appendingPathComponent(source.lastPathComponent)
        guard source.standardizedFileURL != destination.standardizedFileURL else { return destination }

        do {
            try fm.createDirectory(at: folder, withIntermediateDirectories: true)
            if fm.fileExists(atPath: destination.path) {
                try fm.removeItem(at: destination)
            }
            try fm.copyItem(at: source, to: destination)
            return destination
        } catch {
            return nil
        }
    }

    static func moveOwnToTrash(_ url: URL) -> Bool {
        guard ownExtensions.contains(url.pathExtension.lowercased()) else { return false }
        return (try? FileManager.default.trashItem(at: url, resultingItemURL: nil)) != nil
    }

    static func shippedFiles(in folder: URL) -> [URL] {
        files(in: folder).filter { $0.pathExtension.lowercased() == shippedExtension }
    }

    private static func files(in folder: URL) -> [URL] {
        (try? FileManager.default.contentsOfDirectory(
            at: folder,
            includingPropertiesForKeys: nil,
            options: [.skipsHiddenFiles]
        )) ?? []
    }
}
