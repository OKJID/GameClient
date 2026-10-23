import SwiftUI
import AppKit

struct HotkeyPresetAnchorKey: PreferenceKey {
    static var defaultValue: Anchor<CGRect>?

    static func reduce(value: inout Anchor<CGRect>?, nextValue: () -> Anchor<CGRect>?) {
        value = value ?? nextValue()
    }
}

struct HotkeyPresetSelector: View {
    let title: String
    let accent: Color
    let isOpen: Bool
    let isEnabled: Bool
    let action: () -> Void

    var body: some View {
        Button(action: action) {
            HStack(spacing: 10) {
                Image(systemName: "square.stack.3d.up.fill")
                    .font(.system(size: 14, weight: .bold))
                    .foregroundColor(accent)

                VStack(alignment: .leading, spacing: 1) {
                    Text(L10n.hotkeys.preset.uppercased())
                        .font(.system(size: 9, weight: .bold, design: .monospaced))
                        .foregroundColor(.white.opacity(0.45))
                    Text(title)
                        .font(.system(size: 13, weight: .black, design: .monospaced))
                        .foregroundColor(.white)
                        .lineLimit(1)
                }

                Image(systemName: "chevron.down")
                    .font(.system(size: 10, weight: .bold))
                    .foregroundColor(accent)
                    .rotationEffect(.degrees(isOpen ? 180 : 0))
            }
            .padding(.horizontal, 12)
            .padding(.vertical, 6)
            .background(RoundedRectangle(cornerRadius: 6).fill(accent.opacity(isOpen ? 0.24 : 0.14)))
            .overlay(
                RoundedRectangle(cornerRadius: 6)
                    .stroke(accent.opacity(isOpen ? 0.9 : 0.55), lineWidth: 1)
            )
            .contentShape(RoundedRectangle(cornerRadius: 6))
        }
        .buttonStyle(PlainButtonStyle())
        .disabled(!isEnabled)
        .opacity(isEnabled ? 1 : 0.5)
        .onHover { inside in
            if inside { NSCursor.pointingHand.push() } else { NSCursor.pop() }
        }
        .anchorPreference(key: HotkeyPresetAnchorKey.self, value: .bounds) { $0 }
    }
}

struct HotkeyPresetDropdown: View {
    let accent: Color
    let currentName: String?
    let shippedPresets: [HotkeyPresetEntry]
    let ownPresets: [HotkeyPresetEntry]
    let onStandard: () -> Void
    let onPreset: (HotkeyPresetEntry) -> Void
    let onDelete: (HotkeyPresetEntry) -> Void
    let onImport: () -> Void
    let onExport: (String) -> Void
    let onOpenFolder: () -> Void

    private let width: CGFloat = 360
    private let panelColor = Color(red: 0.07, green: 0.08, blue: 0.1)

    var body: some View {
        VStack(alignment: .leading, spacing: 2) {
            HotkeyPresetRow(
                title: L10n.hotkeys.presetStandard,
                systemImage: nil,
                isCurrent: currentName == nil,
                accent: accent,
                action: onStandard
            )
            _buildSection(L10n.hotkeys.presetsShipped, entries: shippedPresets, isDeletable: false)
            _buildSection(L10n.hotkeys.presetsMine, entries: ownPresets, isDeletable: true)
            _buildDivider()
            HotkeyPresetRow(title: L10n.hotkeys.presetImport, systemImage: "square.and.arrow.down", isCurrent: false, accent: accent, action: onImport)
            HotkeyPresetRow(title: L10n.hotkeys.presetSaveCsf, systemImage: "square.and.arrow.up", isCurrent: false, accent: accent) {
                onExport("csf")
            }
            HotkeyPresetRow(title: L10n.hotkeys.presetSaveBig, systemImage: "shippingbox", isCurrent: false, accent: accent) {
                onExport("big")
            }
            HotkeyPresetRow(title: L10n.hotkeys.presetFolder, systemImage: "folder", isCurrent: false, accent: accent, action: onOpenFolder)
        }
        .padding(6)
        .frame(width: width, alignment: .leading)
        .background(RoundedRectangle(cornerRadius: 8).fill(panelColor.opacity(0.98)))
        .overlay(
            RoundedRectangle(cornerRadius: 8)
                .stroke(accent.opacity(0.35), lineWidth: 1)
        )
        .shadow(color: .black.opacity(0.6), radius: 18, y: 8)
    }

    @ViewBuilder
    private func _buildSection(_ title: String, entries: [HotkeyPresetEntry], isDeletable: Bool) -> some View {
        if !entries.isEmpty {
            _buildDivider()
            Text(title.uppercased())
                .font(.system(size: 9, weight: .bold, design: .monospaced))
                .foregroundColor(.white.opacity(0.4))
                .padding(.horizontal, 10)
                .padding(.top, 2)
                .padding(.bottom, 2)
            ForEach(entries) { entry in
                HotkeyPresetRow(
                    title: entry.title,
                    systemImage: nil,
                    isCurrent: entry.name == currentName,
                    accent: accent,
                    onDelete: isDeletable ? { onDelete(entry) } : nil
                ) {
                    onPreset(entry)
                }
            }
        }
    }

    private func _buildDivider() -> some View {
        Rectangle()
            .fill(Color.white.opacity(0.08))
            .frame(height: 1)
            .padding(.vertical, 4)
    }
}

private struct HotkeyPresetRow: View {
    let title: String
    let systemImage: String?
    let isCurrent: Bool
    let accent: Color
    var onDelete: (() -> Void)? = nil
    let action: () -> Void

    @State private var isHovered = false

    var body: some View {
        HStack(spacing: 0) {
            Button(action: action) {
                HStack(spacing: 8) {
                    _buildLeadingMark()
                    Text(title)
                        .font(.system(size: 12, weight: isCurrent ? .black : .semibold, design: .monospaced))
                        .foregroundColor(isCurrent ? accent : .white.opacity(0.85))
                        .lineLimit(1)
                        .truncationMode(.middle)
                    Spacer(minLength: 0)
                }
                .padding(.leading, 10)
                .padding(.trailing, onDelete == nil ? 10 : 4)
                .padding(.vertical, 7)
                .contentShape(Rectangle())
            }
            .buttonStyle(PlainButtonStyle())

            if let onDelete {
                HotkeyPresetDeleteButton(action: onDelete)
            }
        }
        .background(RoundedRectangle(cornerRadius: 5).fill(isHovered ? accent.opacity(0.16) : Color.clear))
        .contentShape(RoundedRectangle(cornerRadius: 5))
        .onHover { inside in
            isHovered = inside
            if inside { NSCursor.pointingHand.push() } else { NSCursor.pop() }
        }
    }

    private func _buildLeadingMark() -> some View {
        Image(systemName: systemImage ?? "checkmark")
            .font(.system(size: 10, weight: .bold))
            .foregroundColor(systemImage == nil ? accent : .white.opacity(0.6))
            .opacity(systemImage != nil || isCurrent ? 1 : 0)
            .frame(width: 14)
    }
}

private struct HotkeyPresetDeleteButton: View {
    let action: () -> Void

    @State private var isHovered = false

    private let danger = Color(red: 1.0, green: 0.38, blue: 0.38)

    var body: some View {
        Button(action: action) {
            Image(systemName: "trash")
                .font(.system(size: 11, weight: .semibold))
                .foregroundColor(isHovered ? danger : .white.opacity(0.4))
                .frame(width: 30, height: 28)
                .contentShape(Rectangle())
        }
        .buttonStyle(PlainButtonStyle())
        .help(L10n.hotkeys.presetDelete)
        .onHover { inside in
            isHovered = inside
            if inside { NSCursor.pointingHand.push() } else { NSCursor.pop() }
        }
    }
}
