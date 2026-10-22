import SwiftUI

extension HotkeyConflict.Kind {
    var titleFormat: String {
        switch self {
        case .dominates: return L10n.hotkeys.conflictDominates
        case .suppressed: return L10n.hotkeys.conflictSuppressed
        case .firesWithGlobal: return L10n.hotkeys.conflictFiresWithGlobal
        case .suppressedByGlobal: return L10n.hotkeys.conflictSuppressedByGlobal
        case .firesWithButton: return L10n.hotkeys.conflictFiresWithButton
        case .suppressesButton: return L10n.hotkeys.conflictSuppressesButton
        case .clashesWithGlobal: return L10n.hotkeys.conflictClashesWithGlobal
        case .takenByMapScroll: return L10n.hotkeys.conflictTakenByMapScroll
        }
    }
}

struct HotkeyDetailCard: View {
    private struct ConflictLine: Identifiable {
        let state: HotkeyConflictState
        let title: String
        let panel: String?
        var sides: [String]

        var id: String { "\(title)|\(panel ?? "")" }
    }

    @ObservedObject var editor: HotkeyEditorModel
    let target: HotkeyEditorModel.Target
    let scope: Set<String>?
    let accent: Color

    private static let noKeyText = "—"

    private let visibleConflicts = 8
    private let visibleSides = 4
    private let width: CGFloat = 320
    private let background = Color(red: 0.08, green: 0.09, blue: 0.1)

    static func shownKey(_ key: String) -> String {
        key.isEmpty ? noKeyText : key
    }

    private var details: String { editor.details(of: target) }

    private var conflicts: [ConflictLine] {
        editor.conflicts(of: target, inSides: scope).reduce(into: [ConflictLine]()) { lines, entry in
            let line = ConflictLine(
                state: entry.conflict.kind.state,
                title: _title(of: entry.conflict),
                panel: entry.conflict.context.map(editor.panelTitle),
                sides: entry.sides.map(editor.sideTitle)
            )
            guard let index = lines.firstIndex(where: { $0.id == line.id }) else {
                lines.append(line)
                return
            }

            lines[index].sides += line.sides.filter { !lines[index].sides.contains($0) }
        }
    }

    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            _buildTitle()

            if editor.isEdited(target) {
                Text(String(format: L10n.hotkeys.original, HotkeyDetailCard.shownKey(editor.originalKeyTitle(of: target))))
                    .font(.system(size: 10, design: .monospaced))
                    .foregroundColor(accent.opacity(0.8))
            }

            if !details.isEmpty {
                Text(details)
                    .font(.system(size: 10, design: .monospaced))
                    .foregroundColor(.white.opacity(0.6))
                    .lineLimit(8)
                    .fixedSize(horizontal: false, vertical: true)
            }

            if !conflicts.isEmpty {
                Divider()
                    .background(Color.white.opacity(0.15))
                _buildConflicts()
            }
        }
        .padding(12)
        .frame(width: width, alignment: .leading)
        .background(background)
        .environment(\.colorScheme, .dark)
    }

    private func _buildTitle() -> some View {
        HStack(alignment: .top, spacing: 8) {
            Text(editor.name(of: target))
                .font(.system(size: 12, weight: .bold, design: .monospaced))
                .foregroundColor(.white)
                .fixedSize(horizontal: false, vertical: true)

            Spacer(minLength: 8)

            HotkeyKeyBadge(
                text: editor.keyTitle(of: target),
                state: editor.state(of: target, inSides: scope),
                isCapturing: false,
                accent: accent
            )
        }
    }

    private func _buildConflicts() -> some View {
        let lines = conflicts

        return VStack(alignment: .leading, spacing: 6) {
            ForEach(lines.prefix(visibleConflicts)) { line in
                _buildConflictRow(line)
            }

            if lines.count > visibleConflicts {
                Text("+\(lines.count - visibleConflicts)")
                    .font(.system(size: 9, weight: .bold, design: .monospaced))
                    .foregroundColor(.white.opacity(0.5))
            }
        }
    }

    private func _buildConflictRow(_ line: ConflictLine) -> some View {
        HStack(alignment: .top, spacing: 6) {
            Circle()
                .fill(line.state.color ?? .white)
                .frame(width: 6, height: 6)
                .padding(.top, 4)

            VStack(alignment: .leading, spacing: 2) {
                Text(line.title)
                    .font(.system(size: 10, design: .monospaced))
                    .foregroundColor(.white.opacity(0.9))
                    .fixedSize(horizontal: false, vertical: true)

                if let subtitle = _subtitle(of: line) {
                    Text(subtitle)
                        .font(.system(size: 9, design: .monospaced))
                        .foregroundColor(.white.opacity(0.5))
                        .lineLimit(2)
                }
            }
        }
    }

    private func _title(of conflict: HotkeyConflict) -> String {
        let format = conflict.kind.titleFormat
        let title = conflict.kind == .takenByMapScroll ? format : String(format: format, editor.otherName(of: conflict))
        let notes = [
            conflict.isOtherHiddenUntilScience ? L10n.hotkeys.afterScience : nil,
            conflict.isOtherOfflineOnly ? L10n.hotkeys.offlineOnly : nil
        ].compactMap { $0 }

        return ([title] + notes).joined(separator: " · ")
    }

    private func _subtitle(of line: ConflictLine) -> String? {
        let hiddenSides = line.sides.count - visibleSides
        let shownSides = line.sides.prefix(visibleSides).joined(separator: ", ") + (hiddenSides > 0 ? " +\(hiddenSides)" : "")
        let parts = [line.panel, line.sides.isEmpty ? nil : shownSides].compactMap { $0 }

        return parts.isEmpty ? nil : parts.joined(separator: " · ")
    }
}
