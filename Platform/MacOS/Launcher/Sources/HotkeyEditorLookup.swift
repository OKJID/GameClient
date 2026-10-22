import Foundation

extension HotkeySection {
    func target(of item: String) -> HotkeyEditorModel.Target {
        kind == .globals ? .global(item) : .label(item)
    }
}

extension HotkeyConflict.Kind {
    var refersToGlobal: Bool {
        switch self {
        case .firesWithGlobal, .suppressedByGlobal, .clashesWithGlobal: return true
        default: return false
        }
    }
}

extension HotkeyEditorModel {
    var allSections: [HotkeySection] {
        sections.flatMap { [$0] + $0.children }
    }

    var selectedSection: HotkeySection? {
        allSections.first { $0.id == selectedSectionID }
    }

    var displayedSections: [HotkeySection] {
        let query = searchText.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !query.isEmpty else { return selectedSection.map { [$0] } ?? [] }

        var shownItems = Set<String>()
        return allSections.compactMap { section in
            let items = section.items.filter { item in
                matches(section.target(of: item), query: query) && shownItems.insert(item).inserted
            }
            guard !items.isEmpty else { return nil }
            return HotkeySection(kind: section.kind, title: section.title, items: items, children: [])
        }
    }

    func scope(of section: HotkeySection) -> Set<String>? {
        switch section.kind {
        case .side(let side): return [side]
        case .faction(let baseSide): return labelUsage?.sides(of: baseSide)
        case .globals, .shared: return nil
        }
    }

    func name(of target: Target) -> String {
        guard let dump else { return "" }

        switch target {
        case .label(let label): return dump.hotkeys[label]?.name ?? label
        case .global(let id): return HotkeyTree.displayName(ofGlobal: id, in: dump)
        }
    }

    func details(of target: Target) -> String {
        switch target {
        case .label(let label): return dump?.hotkeys[label]?.description ?? ""
        case .global(let id): return dump?.globals[id]?.description ?? ""
        }
    }

    func keyTitle(of target: Target) -> String {
        guard let keymap else { return "" }

        switch target {
        case .label(let label): return keymap.key(ofLabel: label)
        case .global(let id): return HotkeyKeyNames.title(ofChord: keymap.chord(ofGlobal: id))
        }
    }

    func originalKeyTitle(of target: Target) -> String {
        switch target {
        case .label(let label):
            return dump?.hotkeys[label]?.key ?? ""
        case .global(let id):
            guard let global = dump?.globals[id] else { return "" }
            return HotkeyKeyNames.title(ofChord: HotkeyEdits.GlobalKeyEdit(key: global.key, modifiers: global.modifiers))
        }
    }

    func isEdited(_ target: Target) -> Bool {
        switch target {
        case .label(let label): return edits.hotkeys[label] != nil
        case .global(let id): return edits.globals[id] != nil
        }
    }

    func state(of target: Target, inSides scope: Set<String>? = nil) -> HotkeyConflictState {
        switch target {
        case .label(let label): return report.state(ofLabel: label, inSides: scope)
        case .global(let id): return report.state(ofGlobal: id)
        }
    }

    func conflicts(of target: Target, inSides scope: Set<String>? = nil) -> [HotkeyConflictEntry] {
        switch target {
        case .label(let label): return report.conflicts(ofLabel: label, inSides: scope)
        case .global(let id): return report.conflicts(ofGlobal: id)
        }
    }

    func zone(ofGlobal id: String) -> HotkeyGlobalZone {
        dump?.globals[id]?.zone ?? .match
    }

    func buttonGroup(ofLabel label: String) -> HotkeyButtonGroup {
        labelUsage?.group(of: label) ?? .other
    }

    func iconURL(of target: Target) -> URL? {
        guard case .label(let label) = target, let icon = dump?.hotkeys[label]?.icon else { return nil }
        return files.iconURL(icon)
    }

    func troubleCount(in section: HotkeySection) -> Int {
        let sides = scope(of: section)
        return section.items.filter { state(of: section.target(of: $0), inSides: sides) >= .firesTwice }.count
    }

    func worstState(in section: HotkeySection) -> HotkeyConflictState {
        let sides = scope(of: section)
        return section.items.map { state(of: section.target(of: $0), inSides: sides) }.max() ?? .none
    }

    func sideTitle(_ side: String) -> String {
        let match = dump?.factions.flatMap(\.sides).first { $0.side == side }
        return match.map(HotkeyTree.title(ofSide:)) ?? side
    }

    func panelTitle(_ panel: String) -> String {
        guard let owners = dump?.panels[panel]?.owners, !owners.isEmpty else { return panel }
        return owners.joined(separator: ", ")
    }

    func otherName(of conflict: HotkeyConflict) -> String {
        name(of: conflict.kind.refersToGlobal ? .global(conflict.other) : .label(conflict.other))
    }

    private func matches(_ target: Target, query: String) -> Bool {
        if name(of: target).localizedCaseInsensitiveContains(query) {
            return true
        }

        return query.count == 1 && keyTitle(of: target).caseInsensitiveCompare(query) == .orderedSame
    }
}
