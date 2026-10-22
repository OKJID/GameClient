import Foundation

struct HotkeySection: Identifiable {
    enum Kind: Equatable {
        case globals
        case shared
        case faction(String)
        case side(String)
    }

    let kind: Kind
    let title: String
    let items: [String]
    let children: [HotkeySection]

    var id: String {
        switch kind {
        case .globals: return "globals"
        case .shared: return "shared"
        case .faction(let baseSide): return "faction:\(baseSide)"
        case .side(let side): return "side:\(side)"
        }
    }
}

enum HotkeyTree {
    private static let missingTextPrefix = "MISSING:"
    private static let pressToken = "BEGIN_"
    private static let releaseToken = "END_"
    private static let unboundKey = "KEY_NONE"
    private static let categoryOrder = ["CONTROL", "SELECTION", "TEAM", "INTERFACE", "MISC"]

    static func sections(
        of dump: HotkeyDump,
        usage: HotkeyLabelUsage,
        globalsTitle: String,
        sharedTitle: String,
        factionTitle: (String) -> String
    ) -> [HotkeySection] {
        let globals = HotkeySection(kind: .globals, title: globalsTitle, items: sortedGlobals(dump), children: [])
        let shared = HotkeySection(
            kind: .shared,
            title: sharedTitle,
            items: usage.sortedByGroup(usage.sharedAcrossFactions),
            children: []
        )
        let factions = dump.factions.map { factionSection($0, title: factionTitle($0.baseSide), usage: usage) }

        return [globals, shared] + factions
    }

    static func displayName(ofGlobal id: String, in dump: HotkeyDump) -> String {
        guard let name = dump.globals[id]?.name, !isMissingText(name) else { return readableName(ofGlobal: id) }
        return name
    }

    static func title(ofSide side: HotkeyDump.Side) -> String {
        isMissingText(side.name) ? side.side : side.name
    }

    static func linkedGlobals(of id: String, in dump: HotkeyDump) -> [String] {
        let release = id.replacingOccurrences(of: pressToken, with: releaseToken)
        guard release != id, dump.globals[release] != nil else { return [id] }

        return [id, release]
    }

    private static func factionSection(_ faction: HotkeyDump.Faction, title: String, usage: HotkeyLabelUsage) -> HotkeySection {
        let sides = faction.sides
            .map { side in
                HotkeySection(
                    kind: .side(side.side),
                    title: HotkeyTree.title(ofSide: side),
                    items: usage.sortedByGroup(usage.labels(of: side.side)),
                    children: []
                )
            }
            .filter { !$0.items.isEmpty }

        return HotkeySection(
            kind: .faction(faction.baseSide),
            title: title,
            items: usage.sortedByGroup(usage.labels(sharedWithin: faction.baseSide)),
            children: sides
        )
    }

    private static func sortedGlobals(_ dump: HotkeyDump) -> [String] {
        dump.globals.keys
            .filter { isEditableGlobal($0, in: dump) }
            .sorted { lhs, rhs in
                let lhsOrder = order(ofGlobal: lhs, in: dump)
                let rhsOrder = order(ofGlobal: rhs, in: dump)
                guard lhsOrder == rhsOrder else { return lhsOrder < rhsOrder }

                let lhsName = displayName(ofGlobal: lhs, in: dump)
                let rhsName = displayName(ofGlobal: rhs, in: dump)
                return lhsName.localizedStandardCompare(rhsName) == .orderedAscending
            }
    }

    private static func order(ofGlobal id: String, in dump: HotkeyDump) -> (zone: Int, category: Int) {
        guard let global = dump.globals[id] else { return (.max, .max) }

        let categoryRank = categoryOrder.firstIndex(of: global.category) ?? categoryOrder.count
        return (global.zone.rawValue, categoryRank)
    }

    private static func isEditableGlobal(_ id: String, in dump: HotkeyDump) -> Bool {
        guard id.allSatisfy({ $0.isLetter || $0.isNumber || $0 == "_" }) else { return false }
        guard dump.globals[id]?.key != unboundKey else { return false }

        let press = id.replacingOccurrences(of: releaseToken, with: pressToken)
        return press == id || dump.globals[press] == nil
    }

    private static func isMissingText(_ text: String) -> Bool {
        text.isEmpty || text.hasPrefix(missingTextPrefix)
    }

    private static func readableName(ofGlobal id: String) -> String {
        let words = id
            .replacingOccurrences(of: pressToken, with: "")
            .replacingOccurrences(of: "_", with: " ")
            .lowercased()
        return words.prefix(1).uppercased() + words.dropFirst()
    }
}

struct HotkeyLabelUsage {
    private let sidesByLabel: [String: Set<String>]
    private let factionBySide: [String: String]
    private let positionByLabel: [String: Int]
    private let groupByLabel: [String: HotkeyButtonGroup]

    init(dump: HotkeyDump) {
        var factionBySide: [String: String] = [:]
        var sidesByLabel: [String: Set<String>] = [:]
        var positionByLabel: [String: Int] = [:]
        var groupByLabel: [String: HotkeyButtonGroup] = [:]
        for faction in dump.factions {
            for side in faction.sides {
                factionBySide[side.side] = faction.baseSide
                for (button, panel) in HotkeyLabelUsage.buttonsInPanelOrder(of: side, in: dump) {
                    let group = HotkeyButtonGroup.of(button, in: panel)
                    sidesByLabel[button.label, default: []].insert(side.side)
                    positionByLabel[button.label] = positionByLabel[button.label] ?? positionByLabel.count
                    groupByLabel[button.label] = groupByLabel[button.label].map { HotkeyButtonGroup.preferred($0, group) } ?? group
                }
            }
        }

        self.sidesByLabel = sidesByLabel
        self.factionBySide = factionBySide
        self.positionByLabel = positionByLabel
        self.groupByLabel = groupByLabel
    }

    var sharedAcrossFactions: [String] {
        sidesByLabel.keys.filter { factions(of: $0).count > 1 }
    }

    func labels(sharedWithin baseSide: String) -> [String] {
        sidesByLabel.keys.filter { label in
            factions(of: label) == [baseSide] && (sidesByLabel[label]?.count ?? 0) > 1
        }
    }

    func labels(of side: String) -> [String] {
        sidesByLabel.keys.filter { sidesByLabel[$0]?.contains(side) == true }
    }

    func sides(of baseSide: String) -> Set<String> {
        Set(factionBySide.filter { $0.value == baseSide }.keys)
    }

    func group(of label: String) -> HotkeyButtonGroup {
        groupByLabel[label] ?? .other
    }

    func sortedByGroup(_ labels: [String]) -> [String] {
        labels.sorted { lhs, rhs in
            let lhsGroup = group(of: lhs)
            let rhsGroup = group(of: rhs)
            guard lhsGroup == rhsGroup else { return lhsGroup < rhsGroup }

            return (positionByLabel[lhs] ?? .max) < (positionByLabel[rhs] ?? .max)
        }
    }

    private func factions(of label: String) -> Set<String> {
        Set((sidesByLabel[label] ?? []).compactMap { factionBySide[$0] })
    }

    private static func buttonsInPanelOrder(
        of side: HotkeyDump.Side,
        in dump: HotkeyDump
    ) -> [(HotkeyDump.PanelButton, HotkeyDump.CommandPanel)] {
        side.panels
            .compactMap { dump.panels[$0] }
            .enumerated()
            .sorted { ($0.element.displayOrder, $0.offset) < ($1.element.displayOrder, $1.offset) }
            .flatMap { entry in
                entry.element.buttons.sorted { $0.slot < $1.slot }.map { ($0, entry.element) }
            }
    }
}
