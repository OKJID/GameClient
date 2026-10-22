import Foundation

enum HotkeyButtonGroup: Int, Comparable {
    case buildings
    case infantry
    case vehicles
    case aircraft
    case heroes
    case upgrades
    case generalPowers
    case structureAbilities
    case unitAbilities
    case orders
    case sciences
    case other

    private static let classificationOrder: [HotkeyButtonGroup] = [
        .generalPowers, .sciences, .buildings, .heroes, .aircraft, .infantry, .vehicles,
        .upgrades, .structureAbilities, .unitAbilities, .orders, .other
    ]

    private static let abilityCommands: Set<String> = [
        "SPECIAL_POWER", "FIRE_WEAPON", "SWITCH_WEAPON", "SPECIAL_POWER_CONSTRUCT", "HIJACK_VEHICLE",
        "CONVERT_TO_CARBOMB", "SABOTAGE_BUILDING", "HACK_INTERNET", "TOGGLE_OVERCHARGE", "COMBATDROP"
    ]

    private static let upgradeCommands: Set<String> = ["PLAYER_UPGRADE", "OBJECT_UPGRADE"]

    static func < (lhs: HotkeyButtonGroup, rhs: HotkeyButtonGroup) -> Bool {
        lhs.rawValue < rhs.rawValue
    }

    static func of(_ button: HotkeyDump.PanelButton, in panel: HotkeyDump.CommandPanel) -> HotkeyButtonGroup {
        if panel.isShortcutPanel { return .generalPowers }
        if panel.isSciencePanel { return .sciences }
        guard let command = button.command else { return .other }

        if command == "DOZER_CONSTRUCT" { return .buildings }
        if command == "UNIT_BUILD" { return production(of: button.objectKinds ?? []) }
        if upgradeCommands.contains(command) { return .upgrades }
        if abilityCommands.contains(command) {
            return panel.isOwnedByStructure ? .structureAbilities : .unitAbilities
        }
        return .orders
    }

    static func preferred(_ lhs: HotkeyButtonGroup, _ rhs: HotkeyButtonGroup) -> HotkeyButtonGroup {
        let lhsRank = classificationOrder.firstIndex(of: lhs) ?? classificationOrder.count
        let rhsRank = classificationOrder.firstIndex(of: rhs) ?? classificationOrder.count
        return lhsRank <= rhsRank ? lhs : rhs
    }

    private static func production(of kinds: [String]) -> HotkeyButtonGroup {
        if kinds.contains("HERO") { return .heroes }
        if kinds.contains("STRUCTURE") { return .buildings }
        if kinds.contains("AIRCRAFT") { return .aircraft }
        if kinds.contains("INFANTRY") { return .infantry }
        return .vehicles
    }
}
