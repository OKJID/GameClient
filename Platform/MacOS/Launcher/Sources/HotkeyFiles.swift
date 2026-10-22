import Foundation

struct HotkeyFiles {
    let directory: URL

    private static let originalSuffix = ".orig"

    var dumpURL: URL { directory.appendingPathComponent("hotkey-dump.json") }
    var editURL: URL { directory.appendingPathComponent("hotkey-dump-edit.json") }

    func iconURL(_ relativePath: String) -> URL {
        directory.appendingPathComponent(relativePath)
    }

    func loadDump() -> HotkeyDump? {
        guard let data = try? Data(contentsOf: dumpURL) else { return nil }
        return try? JSONDecoder().decode(HotkeyDump.self, from: data)
    }

    func dumpModificationDate() -> Date? {
        let attributes = try? FileManager.default.attributesOfItem(atPath: dumpURL.path)
        return attributes?[.modificationDate] as? Date
    }

    func loadEdits() -> HotkeyEdits {
        guard let data = try? Data(contentsOf: editURL),
              let edits = try? JSONDecoder().decode(HotkeyEdits.self, from: data) else {
            return HotkeyEdits()
        }

        return edits
    }

    func saveEdits(_ edits: HotkeyEdits) {
        guard !edits.isEmpty else {
            try? FileManager.default.removeItem(at: editURL)
            return
        }

        guard let data = try? JSONEncoder().encode(edits) else { return }
        try? FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
        try? data.write(to: editURL, options: .atomic)
    }

    func hasOriginals(of dump: HotkeyDump) -> Bool {
        targets(of: dump).contains { FileManager.default.fileExists(atPath: originalURL(of: $0).path) }
    }

    @discardableResult
    func restoreOriginals(of dump: HotkeyDump) -> Int {
        targets(of: dump).filter { restoreOriginal(of: $0) }.count
    }

    private func targets(of dump: HotkeyDump) -> [URL] {
        let root = URL(fileURLWithPath: dump.targets.root)
        let sourceFiles = dump.globals.values.compactMap { $0.source?.file }
        let files = Set([dump.targets.strings, dump.targets.globals] + sourceFiles)

        return files.map { root.appendingPathComponent($0) }
    }

    private func originalURL(of target: URL) -> URL {
        URL(fileURLWithPath: target.path + HotkeyFiles.originalSuffix)
    }

    private func restoreOriginal(of target: URL) -> Bool {
        let original = originalURL(of: target)
        guard FileManager.default.fileExists(atPath: original.path) else { return false }

        try? FileManager.default.removeItem(at: target)
        return (try? FileManager.default.moveItem(at: original, to: target)) != nil
    }
}
