import Foundation

struct HotkeyDump: Decodable {
    struct Profile: Decodable {
        let game: String
        let mod: String?
        let language: String
    }

    struct Targets: Decodable {
        let root: String
        let strings: String
        let globals: String
    }

    struct Source: Decodable {
        let file: String
        let layer: String?
    }

    struct GlobalKey: Decodable {
        let key: String
        let modifiers: String
        let transition: String
        let category: String
        let usableIn: [String]
        let limitedTo: String?
        let keepsKey: Bool?
        let name: String
        let description: String
        let source: Source?
    }

    struct HotkeyString: Decodable {
        let key: String
        let name: String
        let text: String
        let description: String
        let icon: String?
        let source: Source
    }

    struct PanelButton: Decodable {
        let slot: Int
        let button: String
        let label: String
        let needsScience: Bool
        let command: String?
        let border: String?
        let objectKinds: [String]?
    }

    struct CommandPanel: Decodable {
        let kind: String
        let owners: [String]
        let ownerKinds: [String]?
        let buttons: [PanelButton]
    }

    struct Side: Decodable {
        let side: String
        let template: String
        let name: String
        let panels: [String]
    }

    struct Faction: Decodable {
        let baseSide: String
        let sides: [Side]
    }

    let schema: Int
    let profile: Profile
    let targets: Targets
    let globals: [String: GlobalKey]
    let hotkeys: [String: HotkeyString]
    let panels: [String: CommandPanel]
    let factions: [Faction]
}

struct HotkeyEdits: Codable, Equatable {
    struct GlobalKeyEdit: Codable, Equatable {
        var key: String
        var modifiers: String
    }

    var schema = 1
    var hotkeys: [String: String] = [:]
    var globals: [String: GlobalKeyEdit] = [:]

    var isEmpty: Bool { hotkeys.isEmpty && globals.isEmpty }
}

enum HotkeyGlobalZone: Int, Comparable {
    case match
    case offline
    case observer
    case replay
    case everywhere
    case menu

    static func < (lhs: HotkeyGlobalZone, rhs: HotkeyGlobalZone) -> Bool {
        lhs.rawValue < rhs.rawValue
    }
}

extension HotkeyDump.GlobalKey {
    var isUsableInGame: Bool { usableIn.contains("GAME") }
    var isUsableInMenu: Bool { usableIn.contains("SHELL") }
    var isUsableAsObserver: Bool { usableIn.contains("OBSERVER") }
    var isReplayOnly: Bool { limitedTo == "replay" }
    var isOfflineOnly: Bool { limitedTo == "offline" }
    var hasNoModifiers: Bool { modifiers == "NONE" }

    var takesKeyFromButtons: Bool {
        isUsableInGame && !isReplayOnly && keepsKey != true
    }

    var zone: HotkeyGlobalZone {
        if isReplayOnly { return .replay }
        if isOfflineOnly { return .offline }
        if isUsableInMenu && isUsableInGame { return .everywhere }
        if isUsableInGame { return .match }
        if isUsableAsObserver { return .observer }
        return .menu
    }

    func sharesZone(with other: HotkeyDump.GlobalKey) -> Bool {
        if isUsableInMenu && other.isUsableInMenu { return true }
        return isActiveInMatch && other.isActiveInMatch
    }

    private var isActiveInMatch: Bool { isUsableInGame || isUsableAsObserver }
}

extension HotkeyDump.CommandPanel {
    var isObjectPanel: Bool { kind == "object" }
    var isShortcutPanel: Bool { kind == "shortcut" }
    var isSciencePanel: Bool { kind == "science" }

    var displayOrder: Int {
        if isObjectPanel { return 0 }
        if isShortcutPanel { return 1 }
        return 2
    }

    var isOwnedByStructure: Bool { ownerKinds?.contains("STRUCTURE") == true }
}
