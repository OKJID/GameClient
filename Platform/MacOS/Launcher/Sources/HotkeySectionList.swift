import SwiftUI
import AppKit

struct HotkeySectionList: View {
    @ObservedObject var editor: HotkeyEditorModel
    let accent: Color

    @State private var collapsedSectionIDs: Set<String> = []

    private let childIndent: CGFloat = 18

    private var isSearching: Bool {
        !editor.searchText.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty
    }

    var body: some View {
        VStack(spacing: 0) {
            _buildSearchField()
                .padding(12)

            ScrollView(.vertical, showsIndicators: true) {
                VStack(alignment: .leading, spacing: 2) {
                    ForEach(editor.sections) { section in
                        _buildRow(section, indent: 0)

                        if !collapsedSectionIDs.contains(section.id) {
                            ForEach(section.children) { child in
                                _buildRow(child, indent: childIndent)
                            }
                        }
                    }
                }
                .padding(.horizontal, 8)
                .padding(.bottom, 16)
            }
        }
    }

    private func _toggleCollapsed(_ section: HotkeySection) {
        if collapsedSectionIDs.contains(section.id) {
            collapsedSectionIDs.remove(section.id)
        } else {
            collapsedSectionIDs.insert(section.id)
        }
    }

    private func _buildSearchField() -> some View {
        HStack(spacing: 6) {
            Image(systemName: "magnifyingglass")
                .font(.system(size: 11))
                .foregroundColor(.white.opacity(0.4))

            TextField(L10n.hotkeys.search, text: $editor.searchText)
                .textFieldStyle(PlainTextFieldStyle())
                .font(.system(size: 11, design: .monospaced))
                .foregroundColor(.white)
        }
        .padding(.horizontal, 10)
        .padding(.vertical, 7)
        .background(RoundedRectangle(cornerRadius: 6).fill(Color.white.opacity(0.05)))
        .overlay(
            RoundedRectangle(cornerRadius: 6)
                .stroke(Color.white.opacity(0.08), lineWidth: 1)
        )
    }

    private func _buildRow(_ section: HotkeySection, indent: CGFloat) -> some View {
        let isSelected = !isSearching && editor.selectedSectionID == section.id

        return Button(action: { editor.select(section) }) {
            HStack(spacing: 6) {
                _buildDisclosure(section)

                Text(section.title)
                    .font(.system(size: 11, weight: isSelected ? .bold : .regular, design: .monospaced))
                    .foregroundColor(isSelected ? accent : .white.opacity(0.85))
                    .lineLimit(1)

                Spacer(minLength: 6)

                Text("\(section.items.count)")
                    .font(.system(size: 10, design: .monospaced))
                    .foregroundColor(.white.opacity(0.35))

                _buildTroubleBadge(section)
            }
            .padding(.leading, 6 + indent)
            .padding(.trailing, 8)
            .padding(.vertical, 6)
            .background(RoundedRectangle(cornerRadius: 5).fill(isSelected ? accent.opacity(0.15) : Color.clear))
            .contentShape(Rectangle())
        }
        .buttonStyle(PlainButtonStyle())
    }

    @ViewBuilder
    private func _buildDisclosure(_ section: HotkeySection) -> some View {
        if section.children.isEmpty {
            Color.clear.frame(width: 10, height: 10)
        } else {
            Image(systemName: collapsedSectionIDs.contains(section.id) ? "chevron.right" : "chevron.down")
                .font(.system(size: 8, weight: .bold))
                .foregroundColor(.white.opacity(0.5))
                .frame(width: 10, height: 10)
                .contentShape(Rectangle())
                .onTapGesture { _toggleCollapsed(section) }
        }
    }

    @ViewBuilder
    private func _buildTroubleBadge(_ section: HotkeySection) -> some View {
        let count = editor.troubleCount(in: section)
        if count > 0, let color = editor.worstState(in: section).color {
            Text("\(count)")
                .font(.system(size: 9, weight: .black, design: .monospaced))
                .foregroundColor(.black)
                .padding(.horizontal, 5)
                .padding(.vertical, 1)
                .background(Capsule().fill(color))
        }
    }
}
