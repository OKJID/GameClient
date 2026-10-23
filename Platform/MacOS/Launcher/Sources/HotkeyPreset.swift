import Foundation

struct HotkeyPreset: Decodable, Equatable {
    var name: String
    var author: String?
    var version: Int?
    var game: String?
    var hotkeys: [String: String]
    var globals: [String: HotkeyEdits.GlobalKeyEdit]

    var title: String {
        guard let author, !author.isEmpty, author != name else { return name }
        return "\(name) — \(author)"
    }

    func suits(_ profile: HotkeyDump.Profile) -> Bool {
        game == nil || game == profile.game
    }
}

extension HotkeyPreset {
    private enum CodingKeys: String, CodingKey {
        case name, author, version, game, hotkeys, globals
    }

    init(from decoder: Decoder) throws {
        let container = try decoder.container(keyedBy: CodingKeys.self)
        name = try container.decode(String.self, forKey: .name)
        author = try container.decodeIfPresent(String.self, forKey: .author)
        version = try container.decodeIfPresent(Int.self, forKey: .version)
        game = try container.decodeIfPresent(String.self, forKey: .game)
        hotkeys = try container.decodeIfPresent([String: String].self, forKey: .hotkeys) ?? [:]
        globals = try container.decodeIfPresent([String: HotkeyEdits.GlobalKeyEdit].self, forKey: .globals) ?? [:]
    }
}

struct HotkeyPresetEntry: Identifiable {
    enum Kind {
        case shipped(HotkeyPreset)
        case own
    }

    let url: URL
    let kind: Kind

    var id: String { url.path }

    var title: String {
        switch kind {
        case .shipped(let preset): return preset.title
        case .own: return url.lastPathComponent
        }
    }

    var name: String {
        switch kind {
        case .shipped(let preset): return preset.name
        case .own: return url.lastPathComponent
        }
    }

    var isShipped: Bool {
        if case .shipped = kind { return true }
        return false
    }
}

struct HotkeyPresetState: Codable, Equatable {
    var name: String?
    var isModified: Bool
    var root: String?

    func belongs(to installRoot: String) -> Bool {
        root == installRoot
    }
}

enum HotkeyPresetOutcome: Equatable {
    case saved(fileName: String)
    case saveFailed(fileName: String)
    case importFailed(fileName: String)
    case deleteFailed(fileName: String)
}

enum HotkeyPresetMatcher {
    private static let buttonLetters = Set("ABCDEFGHIJKLMNOPQRSTUVWXYZ".map(String.init))

    static func edits(of preset: HotkeyPreset) -> HotkeyEdits {
        var edits = HotkeyEdits()
        edits.hotkeys = preset.hotkeys.filter { isButtonLetter($0.value) }
        edits.globals = preset.globals
        return edits
    }

    static func merged(_ preset: HotkeyPreset, into edits: HotkeyEdits, against dump: HotkeyDump) -> HotkeyEdits {
        var merged = edits
        mergeButtons(of: preset, into: &merged, against: dump)
        mergeGlobals(of: preset, into: &merged, against: dump)
        return merged
    }

    private static func mergeButtons(of preset: HotkeyPreset, into edits: inout HotkeyEdits, against dump: HotkeyDump) {
        let letters = Dictionary(preset.hotkeys.map { ($0.key.lowercased(), $0.value) }, uniquingKeysWith: { first, _ in first })

        for (label, hotkey) in dump.hotkeys {
            guard let letter = letters[label.lowercased()], isButtonLetter(letter) else { continue }
            edits.hotkeys[label] = letter == hotkey.key ? nil : letter
        }
    }

    private static func mergeGlobals(of preset: HotkeyPreset, into edits: inout HotkeyEdits, against dump: HotkeyDump) {
        for (id, global) in dump.globals {
            guard let chord = preset.globals[id] else { continue }

            let original = HotkeyEdits.GlobalKeyEdit(key: global.key, modifiers: global.modifiers)
            edits.globals[id] = chord == original ? nil : chord
        }
    }

    private static func isButtonLetter(_ letter: String) -> Bool {
        letter.isEmpty || buttonLetters.contains(letter)
    }
}
