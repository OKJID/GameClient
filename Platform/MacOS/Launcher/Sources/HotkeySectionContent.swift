import SwiftUI

extension HotkeyConflictState {
    var legendTitle: String {
        switch self {
        case .none: return ""
        case .dominant: return L10n.hotkeys.legendDominant
        case .firesTwice: return L10n.hotkeys.legendFiresTwice
        case .suppressed: return L10n.hotkeys.legendSuppressed
        }
    }
}

extension HotkeyGlobalZone {
    var title: String {
        switch self {
        case .match: return L10n.hotkeys.zoneMatch
        case .offline: return L10n.hotkeys.zoneOffline
        case .observer: return L10n.hotkeys.zoneObserver
        case .replay: return L10n.hotkeys.zoneReplay
        case .everywhere: return L10n.hotkeys.zoneEverywhere
        case .menu: return L10n.hotkeys.zoneMenu
        }
    }
}

extension HotkeyButtonGroup {
    var title: String {
        switch self {
        case .buildings: return L10n.hotkeys.groupBuildings
        case .infantry: return L10n.hotkeys.groupInfantry
        case .vehicles: return L10n.hotkeys.groupVehicles
        case .aircraft: return L10n.hotkeys.groupAircraft
        case .heroes: return L10n.hotkeys.groupHeroes
        case .upgrades: return L10n.hotkeys.groupUpgrades
        case .generalPowers: return L10n.hotkeys.groupGeneralPowers
        case .structureAbilities: return L10n.hotkeys.groupStructureAbilities
        case .unitAbilities: return L10n.hotkeys.groupUnitAbilities
        case .orders: return L10n.hotkeys.groupOrders
        case .sciences: return L10n.hotkeys.groupSciences
        case .other: return L10n.hotkeys.groupOther
        }
    }
}

struct HotkeySectionContent: View {
    @ObservedObject var editor: HotkeyEditorModel
    let accent: Color

    private let legendStates: [HotkeyConflictState] = [.suppressed, .firesTwice, .dominant]
    private let iconSpacing: CGFloat = 6
    private let compactMinWidth: CGFloat = 240
    private let compactSpacing: CGFloat = 8
    private let contentInset: CGFloat = 24

    private var iconColumns: [GridItem] {
        let width = HotkeyIconCell.size.width
        return [GridItem(.adaptive(minimum: width, maximum: width), spacing: iconSpacing)]
    }

    private var compactColumns: [GridItem] {
        [GridItem(.adaptive(minimum: compactMinWidth), spacing: compactSpacing)]
    }

    private var hint: String {
        switch editor.capturingTarget {
        case nil: return L10n.hotkeys.hintIdle
        case .label?: return L10n.hotkeys.captureHintLabel
        case .global?: return L10n.hotkeys.captureHintGlobal
        }
    }

    var body: some View {
        VStack(spacing: 0) {
            _buildLegend()
                .padding(.horizontal, contentInset)
                .padding(.vertical, 12)

            ScrollView(.vertical, showsIndicators: true) {
                LazyVStack(alignment: .leading, spacing: 28) {
                    ForEach(editor.displayedSections) { section in
                        _buildSection(section)
                    }
                }
                .padding(.horizontal, contentInset)
                .padding(.bottom, 32)
            }
            .contentShape(Rectangle())
            .onTapGesture(perform: editor.stopCapture)
        }
    }

    private func _buildLegend() -> some View {
        HStack(spacing: 14) {
            ForEach(legendStates, id: \.self) { state in
                HStack(spacing: 5) {
                    RoundedRectangle(cornerRadius: 3)
                        .fill(state.color ?? .clear)
                        .frame(width: 14, height: 12)
                    Text(state.legendTitle)
                        .font(.system(size: 10, design: .monospaced))
                        .foregroundColor(.white.opacity(0.6))
                }
            }

            Spacer(minLength: 12)

            Text(hint)
                .font(.system(size: 10, weight: editor.capturingTarget == nil ? .regular : .bold, design: .monospaced))
                .foregroundColor(editor.capturingTarget == nil ? .white.opacity(0.45) : accent)
                .lineLimit(1)
        }
    }

    private func _buildSection(_ section: HotkeySection) -> some View {
        VStack(alignment: .leading, spacing: 12) {
            SettingsSectionHeader(title: "\(section.title) · \(section.items.count)", accent: accent)

            if section.kind == .globals {
                _buildGlobals(section.items)
            } else {
                _buildLabels(section.items, scope: editor.scope(of: section))
            }
        }
    }

    private func _buildGlobals(_ ids: [String]) -> some View {
        let zones = orderedUnique(ids.map(editor.zone(ofGlobal:)))

        return ForEach(zones, id: \.self) { zone in
            _buildSubheader(zone.title)
            _buildCompactGrid(ids.filter { editor.zone(ofGlobal: $0) == zone }.map { .global($0) }, scope: nil)
        }
    }

    private func _buildLabels(_ labels: [String], scope: Set<String>?) -> some View {
        let groups = orderedUnique(labels.map(editor.buttonGroup(ofLabel:)))

        return ForEach(groups, id: \.self) { group in
            _buildSubheader(group.title)
            _buildLabelGroup(labels.filter { editor.buttonGroup(ofLabel: $0) == group }, scope: scope)
        }
    }

    @ViewBuilder
    private func _buildLabelGroup(_ labels: [String], scope: Set<String>?) -> some View {
        let targets = labels.map { HotkeyEditorModel.Target.label($0) }
        let iconTargets = targets.filter { editor.iconURL(of: $0) != nil }
        let compactTargets = targets.filter { editor.iconURL(of: $0) == nil }

        if !iconTargets.isEmpty {
            LazyVGrid(columns: iconColumns, alignment: .leading, spacing: iconSpacing) {
                ForEach(iconTargets, id: \.self) { target in
                    HotkeyIconCell(editor: editor, target: target, scope: scope, accent: accent)
                }
            }
        }

        if !compactTargets.isEmpty {
            _buildCompactGrid(compactTargets, scope: scope)
        }
    }

    private func orderedUnique<Value: Equatable>(_ values: [Value]) -> [Value] {
        values.reduce(into: [Value]()) { unique, value in
            if !unique.contains(value) {
                unique.append(value)
            }
        }
    }

    private func _buildCompactGrid(_ targets: [HotkeyEditorModel.Target], scope: Set<String>?) -> some View {
        LazyVGrid(columns: compactColumns, alignment: .leading, spacing: compactSpacing) {
            ForEach(targets, id: \.self) { target in
                HotkeyCompactCell(editor: editor, target: target, scope: scope, accent: accent)
            }
        }
    }

    private func _buildSubheader(_ title: String) -> some View {
        Text(title.uppercased())
            .font(.system(size: 9, weight: .black, design: .monospaced))
            .foregroundColor(.white.opacity(0.4))
            .padding(.top, 4)
    }
}
