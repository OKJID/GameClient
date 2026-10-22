import Foundation

struct HotkeyConflict: Hashable {
    enum Kind: Hashable {
        case dominates
        case suppressed
        case firesWithGlobal
        case suppressedByGlobal
        case firesWithButton
        case suppressesButton
        case clashesWithGlobal
        case takenByMapScroll
    }

    let kind: Kind
    let key: String
    let other: String
    let context: String?
    var isOtherHiddenUntilScience = false
    var isOtherOfflineOnly = false
}

enum HotkeyConflictState: Int, Comparable {
    case none
    case dominant
    case firesTwice
    case suppressed

    static func < (lhs: HotkeyConflictState, rhs: HotkeyConflictState) -> Bool {
        lhs.rawValue < rhs.rawValue
    }
}

extension HotkeyConflict.Kind {
    var state: HotkeyConflictState {
        switch self {
        case .dominates, .suppressesButton: return .dominant
        case .firesWithGlobal, .firesWithButton, .clashesWithGlobal: return .firesTwice
        case .suppressed, .suppressedByGlobal, .takenByMapScroll: return .suppressed
        }
    }
}

struct HotkeyConflictEntry: Identifiable {
    let conflict: HotkeyConflict
    let sides: [String]

    var id: HotkeyConflict { conflict }
}

struct HotkeyConflictReport {
    private var labelConflicts: [String: [HotkeyConflict: Set<String>]] = [:]
    private var globalConflicts: [String: [HotkeyConflict: Set<String>]] = [:]

    func state(ofLabel label: String, inSides scope: Set<String>? = nil) -> HotkeyConflictState {
        worstState(scoped(labelConflicts[label], to: scope))
    }

    func state(ofGlobal id: String) -> HotkeyConflictState {
        worstState(globalConflicts[id])
    }

    func conflicts(ofLabel label: String, inSides scope: Set<String>? = nil) -> [HotkeyConflictEntry] {
        entries(scoped(labelConflicts[label], to: scope))
    }

    func conflicts(ofGlobal id: String) -> [HotkeyConflictEntry] {
        entries(globalConflicts[id])
    }

    mutating func add(label: String, _ conflict: HotkeyConflict, side: String?) {
        labelConflicts[label, default: [:]][conflict, default: []].formUnion(side.map { [$0] } ?? [])
    }

    mutating func add(global id: String, _ conflict: HotkeyConflict) {
        globalConflicts[id, default: [:]][conflict, default: []] = []
    }

    private func scoped(_ conflicts: [HotkeyConflict: Set<String>]?, to scope: Set<String>?) -> [HotkeyConflict: Set<String>]? {
        guard let scope else { return conflicts }

        return conflicts?.compactMapValues { sides in
            guard !sides.isEmpty else { return sides }

            let sidesInScope = sides.intersection(scope)
            return sidesInScope.isEmpty ? nil : sidesInScope
        }
    }

    private func worstState(_ conflicts: [HotkeyConflict: Set<String>]?) -> HotkeyConflictState {
        conflicts?.keys.map(\.kind.state).max() ?? .none
    }

    private func entries(_ conflicts: [HotkeyConflict: Set<String>]?) -> [HotkeyConflictEntry] {
        (conflicts ?? [:])
            .map { HotkeyConflictEntry(conflict: $0.key, sides: $0.value.sorted()) }
            .sorted { $0.conflict.kind.state > $1.conflict.kind.state }
    }
}

struct HotkeyKeymap {
    let dump: HotkeyDump
    let edits: HotkeyEdits

    func key(ofLabel label: String) -> String {
        edits.hotkeys[label] ?? dump.hotkeys[label]?.key ?? ""
    }

    func chord(ofGlobal id: String) -> HotkeyEdits.GlobalKeyEdit {
        if let edit = edits.globals[id] { return edit }
        let global = dump.globals[id]
        return HotkeyEdits.GlobalKeyEdit(key: global?.key ?? "", modifiers: global?.modifiers ?? "")
    }
}

enum HotkeyConflictFinder {
    private struct PanelEntry {
        let label: String
        let needsScience: Bool
        let isScience: Bool
    }

    private static let mapScrollKeys: Set<String> = ["W", "A", "S", "D"]
    private static let keyPrefix = "KEY_"
    private static let noModifiers = "NONE"
    private static let unboundKey = "KEY_NONE"
    private static let releaseTransition = "UP"

    static func report(keymap: HotkeyKeymap, isMapScrollOnWASD: Bool) -> HotkeyConflictReport {
        var report = HotkeyConflictReport()
        for side in keymap.dump.factions.flatMap(\.sides) {
            addSideConflicts(side, keymap: keymap, into: &report)
        }

        addGlobalButtonConflicts(keymap, into: &report)
        addGlobalClashes(keymap, into: &report)
        if isMapScrollOnWASD {
            addMapScrollConflicts(keymap, into: &report)
        }
        return report
    }

    static func letter(ofKey keyName: String) -> String? {
        guard keyName.hasPrefix(keyPrefix) else { return nil }

        let letter = String(keyName.dropFirst(keyPrefix.count))
        return letter.count == 1 ? letter : nil
    }

    private static func addSideConflicts(_ side: HotkeyDump.Side, keymap: HotkeyKeymap, into report: inout HotkeyConflictReport) {
        let panels = side.panels.compactMap { name in keymap.dump.panels[name].map { (name, $0) } }
        let shortcut = entries(of: panels.filter { $0.1.isShortcutPanel }, isScience: false)
        let science = entries(of: panels.filter { $0.1.isSciencePanel }, isScience: true)

        if let scienceContext = panels.first(where: { $0.1.isSciencePanel })?.0 {
            addDomainConflicts(science, context: scienceContext, side: side.side, skipsSciencePairs: false, keymap: keymap, into: &report)
        }

        for (name, panel) in panels where panel.isObjectPanel {
            let domain = entries(of: [(name, panel)], isScience: false) + shortcut + science
            addDomainConflicts(domain, context: name, side: side.side, skipsSciencePairs: true, keymap: keymap, into: &report)
        }
    }

    private static func entries(of panels: [(String, HotkeyDump.CommandPanel)], isScience: Bool) -> [PanelEntry] {
        panels.flatMap { panel in
            panel.1.buttons.map { PanelEntry(label: $0.label, needsScience: $0.needsScience, isScience: isScience) }
        }
    }

    private static func addDomainConflicts(
        _ entries: [PanelEntry],
        context: String,
        side: String,
        skipsSciencePairs: Bool,
        keymap: HotkeyKeymap,
        into report: inout HotkeyConflictReport
    ) {
        var dominantByKey: [String: PanelEntry] = [:]
        for entry in entries {
            let key = keymap.key(ofLabel: entry.label)
            guard !key.isEmpty else { continue }
            guard let dominant = dominantByKey[key] else {
                dominantByKey[key] = entry
                continue
            }
            guard dominant.label != entry.label, !(skipsSciencePairs && dominant.isScience && entry.isScience) else { continue }

            report.add(label: dominant.label, HotkeyConflict(kind: .dominates, key: key, other: entry.label, context: context), side: side)
            let suppressed = HotkeyConflict(kind: .suppressed, key: key, other: dominant.label, context: context, isOtherHiddenUntilScience: dominant.needsScience)
            report.add(label: entry.label, suppressed, side: side)
        }
    }

    private static func addGlobalButtonConflicts(_ keymap: HotkeyKeymap, into report: inout HotkeyConflictReport) {
        let labelsByKey = Dictionary(grouping: keymap.dump.hotkeys.keys) { keymap.key(ofLabel: $0) }
        for (id, global) in keymap.dump.globals where global.takesKeyFromButtons {
            let chord = keymap.chord(ofGlobal: id)
            guard chord.modifiers == noModifiers, let letter = letter(ofKey: chord.key) else { continue }

            let isRelease = global.transition == releaseTransition
            for label in labelsByKey[letter] ?? [] {
                var labelConflict = HotkeyConflict(kind: isRelease ? .suppressedByGlobal : .firesWithGlobal, key: letter, other: id, context: nil)
                labelConflict.isOtherOfflineOnly = global.isOfflineOnly
                report.add(label: label, labelConflict, side: nil)
                report.add(global: id, HotkeyConflict(kind: isRelease ? .suppressesButton : .firesWithButton, key: letter, other: label, context: nil))
            }
        }
    }

    private static func addGlobalClashes(_ keymap: HotkeyKeymap, into report: inout HotkeyConflictReport) {
        let boundGlobals = keymap.dump.globals.filter { keymap.chord(ofGlobal: $0.key).key != unboundKey }
        let idsByChord = Dictionary(grouping: boundGlobals.keys) { id -> String in
            let chord = keymap.chord(ofGlobal: id)
            return "\(chord.key)+\(chord.modifiers)+\(boundGlobals[id]?.transition ?? "")"
        }

        for ids in idsByChord.values where ids.count > 1 {
            for id in ids {
                guard let global = boundGlobals[id] else { continue }

                let key = keymap.chord(ofGlobal: id).key
                let others = ids.filter { other in
                    other != id && boundGlobals[other].map(global.sharesZone(with:)) == true
                }
                others.forEach { report.add(global: id, HotkeyConflict(kind: .clashesWithGlobal, key: key, other: $0, context: nil)) }
            }
        }
    }

    private static func addMapScrollConflicts(_ keymap: HotkeyKeymap, into report: inout HotkeyConflictReport) {
        for label in keymap.dump.hotkeys.keys where mapScrollKeys.contains(keymap.key(ofLabel: label)) {
            report.add(label: label, HotkeyConflict(kind: .takenByMapScroll, key: keymap.key(ofLabel: label), other: "", context: nil), side: nil)
        }

        for (id, global) in keymap.dump.globals where global.isUsableInGame {
            let chord = keymap.chord(ofGlobal: id)
            guard chord.modifiers == noModifiers, let letter = letter(ofKey: chord.key), mapScrollKeys.contains(letter) else { continue }

            report.add(global: id, HotkeyConflict(kind: .takenByMapScroll, key: letter, other: "", context: nil))
        }
    }
}
