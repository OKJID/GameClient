import Foundation

struct HotkeyFiles {
    let directory: URL

    private static let originalSuffix = ".orig"

    var dumpURL: URL { directory.appendingPathComponent("hotkey-dump.json") }
    var editURL: URL { directory.appendingPathComponent("hotkey-dump-edit.json") }
    var presetStateURL: URL { directory.appendingPathComponent("hotkey-preset.json") }
    var importURL: URL { directory.appendingPathComponent("hotkey-import.json") }

    func iconURL(_ relativePath: String) -> URL {
        directory.appendingPathComponent(relativePath)
    }

    func loadDump() -> HotkeyDump? {
        guard let data = try? Data(contentsOf: dumpURL) else { return nil }
        return try? JSONDecoder().decode(HotkeyDump.self, from: data)
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

    func loadPresetState() -> HotkeyPresetState? {
        guard let data = try? Data(contentsOf: presetStateURL) else { return nil }
        return try? JSONDecoder().decode(HotkeyPresetState.self, from: data)
    }

    func savePresetState(_ state: HotkeyPresetState) {
        guard let data = try? JSONEncoder().encode(state) else { return }
        try? data.write(to: presetStateURL, options: .atomic)
    }

    func removePresetState() {
        try? FileManager.default.removeItem(at: presetStateURL)
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

extension GameProfile {
    private static let hotkeysDirName = "Hotkeys"
    private static let hotkeyPresetsDirName = "Presets"
    private static let baseHotkeysDirName = "base"

    var hotkeyPresetsDirURL: URL {
        hotkeysDirURL.appendingPathComponent(GameProfile.hotkeyPresetsDirName, isDirectory: true)
    }

    func hotkeyWorkDirURL(language: String) -> URL {
        hotkeysDirURL
            .appendingPathComponent(mod?.dirName ?? GameProfile.baseHotkeysDirName, isDirectory: true)
            .appendingPathComponent(language, isDirectory: true)
    }

    private var hotkeysDirURL: URL {
        userDataDirURL.appendingPathComponent(GameProfile.hotkeysDirName, isDirectory: true)
    }
}

enum HotkeyLegacyFiles {
    private static let removedKey = "HOTKEY_WORK_FILES_LEFT_GAME_RESOURCES"
    private static let names = ["hotkey-dump.json", "hotkey-dump-edit.json", "hotkey-preset.json", "hotkey-icons"]

    static var isRemoved: Bool {
        UserDefaults.standard.bool(forKey: removedKey)
    }

    static func remove(fromDataDirectories dataDirectories: [URL]) {
        DispatchQueue.global(qos: .utility).async {
            dataDirectories.forEach(removeFiles(in:))

            DispatchQueue.main.async {
                UserDefaults.standard.set(true, forKey: removedKey)
            }
        }
    }

    private static func removeFiles(in dataDirectory: URL) {
        let fm = FileManager.default
        let languages = (try? fm.contentsOfDirectory(at: dataDirectory, includingPropertiesForKeys: nil)) ?? []
        for language in languages {
            names.forEach { try? fm.removeItem(at: language.appendingPathComponent($0)) }
        }
    }
}
