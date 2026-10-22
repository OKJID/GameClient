import SwiftUI
import AppKit

extension HotkeyConflictState {
    var color: Color? {
        switch self {
        case .none: return nil
        case .dominant: return Color(red: 1.0, green: 0.55, blue: 0.15)
        case .firesTwice: return Color(red: 0.95, green: 0.8, blue: 0.2)
        case .suppressed: return Color(red: 0.95, green: 0.25, blue: 0.25)
        }
    }
}

struct HotkeyKeyBadge: View {
    let text: String
    let state: HotkeyConflictState
    let isCapturing: Bool
    let accent: Color

    private static let emptyText = "+"
    private static let capturingText = "?"

    private var shownText: String {
        if isCapturing { return HotkeyKeyBadge.capturingText }
        return text.isEmpty ? HotkeyKeyBadge.emptyText : text
    }

    private var fill: Color {
        if isCapturing { return accent }
        return state.color ?? Color.black.opacity(0.8)
    }

    private var foreground: Color {
        if isCapturing || state != .none { return .black }
        return text.isEmpty ? .white.opacity(0.35) : .white
    }

    private var border: Color {
        state == .none ? Color.white.opacity(0.2) : Color.black.opacity(0.35)
    }

    var body: some View {
        Text(shownText)
            .font(.system(size: 11, weight: .black, design: .monospaced))
            .foregroundColor(foreground)
            .lineLimit(1)
            .padding(.horizontal, 5)
            .padding(.vertical, 2)
            .frame(minWidth: 18)
            .background(RoundedRectangle(cornerRadius: 3).fill(fill))
            .overlay(
                RoundedRectangle(cornerRadius: 3)
                    .stroke(border, lineWidth: 1)
            )
    }
}

struct HotkeyUnsavedStripes: Shape {
    let spacing: CGFloat

    func path(in rect: CGRect) -> Path {
        var path = Path()
        var x = rect.minX - rect.height
        while x < rect.maxX {
            path.move(to: CGPoint(x: x, y: rect.maxY))
            path.addLine(to: CGPoint(x: x + rect.height, y: rect.minY))
            x += spacing
        }
        return path
    }
}

struct HotkeyStateBorder: View {
    let state: HotkeyConflictState
    let isCapturing: Bool
    let accent: Color
    let cornerRadius: CGFloat

    private var color: Color {
        if isCapturing { return accent }
        return state.color ?? Color.white.opacity(0.08)
    }

    var body: some View {
        RoundedRectangle(cornerRadius: cornerRadius)
            .stroke(color, lineWidth: isCapturing || state != .none ? 2 : 1)
    }
}

struct HotkeyIconCell: View {
    @ObservedObject var editor: HotkeyEditorModel
    let target: HotkeyEditorModel.Target
    let scope: Set<String>?
    let accent: Color

    static let size = CGSize(width: 72, height: 58)

    @State private var image: NSImage?

    private let cornerRadius: CGFloat = 4

    private var state: HotkeyConflictState { editor.state(of: target, inSides: scope) }
    private var isCapturing: Bool { editor.capturingTarget == target }

    var body: some View {
        ZStack(alignment: .bottomTrailing) {
            _buildIcon()

            if editor.isEdited(target) {
                HotkeyUnsavedStripes(spacing: 8)
                    .stroke(accent.opacity(0.6), lineWidth: 2.5)
            }

            HotkeyKeyBadge(
                text: editor.keyTitle(of: target),
                state: state,
                isCapturing: isCapturing,
                accent: accent
            )
            .padding(3)
        }
        .frame(width: HotkeyIconCell.size.width, height: HotkeyIconCell.size.height)
        .clipShape(RoundedRectangle(cornerRadius: cornerRadius))
        .overlay(HotkeyStateBorder(state: state, isCapturing: isCapturing, accent: accent, cornerRadius: cornerRadius))
        .modifier(HotkeyCellInteraction(editor: editor, target: target, scope: scope, accent: accent))
        .task(id: editor.iconURL(of: target)) {
            image = editor.iconURL(of: target).flatMap(NSImage.init(contentsOf:))
        }
    }

    @ViewBuilder
    private func _buildIcon() -> some View {
        if let image {
            Image(nsImage: image)
                .resizable()
                .aspectRatio(contentMode: .fill)
                .frame(width: HotkeyIconCell.size.width, height: HotkeyIconCell.size.height)
                .clipped()
        } else {
            Color.white.opacity(0.05)
        }
    }
}

struct HotkeyCompactCell: View {
    @ObservedObject var editor: HotkeyEditorModel
    let target: HotkeyEditorModel.Target
    let scope: Set<String>?
    let accent: Color

    private let cornerRadius: CGFloat = 4

    private var state: HotkeyConflictState { editor.state(of: target, inSides: scope) }
    private var isCapturing: Bool { editor.capturingTarget == target }

    var body: some View {
        HStack(spacing: 8) {
            Text(editor.name(of: target))
                .font(.system(size: 11, design: .monospaced))
                .foregroundColor(.white.opacity(0.85))
                .lineLimit(2)
                .multilineTextAlignment(.leading)

            Spacer(minLength: 4)

            HotkeyKeyBadge(
                text: editor.keyTitle(of: target),
                state: state,
                isCapturing: isCapturing,
                accent: accent
            )
        }
        .padding(.horizontal, 10)
        .padding(.vertical, 6)
        .frame(maxWidth: .infinity, minHeight: 40, alignment: .leading)
        .background(_buildBackground())
        .clipShape(RoundedRectangle(cornerRadius: cornerRadius))
        .overlay(HotkeyStateBorder(state: state, isCapturing: isCapturing, accent: accent, cornerRadius: cornerRadius))
        .modifier(HotkeyCellInteraction(editor: editor, target: target, scope: scope, accent: accent))
    }

    private func _buildBackground() -> some View {
        ZStack {
            Color.white.opacity(0.03)

            if editor.isEdited(target) {
                HotkeyUnsavedStripes(spacing: 10)
                    .stroke(accent.opacity(0.3), lineWidth: 3)
            }
        }
    }
}

struct HotkeyCellInteraction: ViewModifier {
    @ObservedObject var editor: HotkeyEditorModel
    let target: HotkeyEditorModel.Target
    let scope: Set<String>?
    let accent: Color

    @State private var isHovered = false
    @State private var isDetailShown = false

    private let detailDelayNanoseconds: UInt64 = 350_000_000

    func body(content: Content) -> some View {
        content
            .contentShape(Rectangle())
            .onTapGesture { _toggleCapture() }
            .onHover(perform: _hover)
            .popover(isPresented: $isDetailShown, arrowEdge: .bottom) {
                HotkeyDetailCard(editor: editor, target: target, scope: scope, accent: accent)
            }
            .contextMenu { _buildMenu() }
    }

    private func _toggleCapture() {
        isDetailShown = false
        editor.toggleCapture(target)
    }

    private func _hover(_ inside: Bool) {
        isHovered = inside
        if inside { NSCursor.pointingHand.push() } else { NSCursor.pop() }

        guard inside else {
            isDetailShown = false
            return
        }

        Task { @MainActor in
            try? await Task.sleep(nanoseconds: detailDelayNanoseconds)
            if isHovered {
                isDetailShown = true
            }
        }
    }

    @ViewBuilder
    private func _buildMenu() -> some View {
        if editor.isEdited(target) {
            Button(String(format: L10n.hotkeys.revert, HotkeyDetailCard.shownKey(editor.originalKeyTitle(of: target)))) {
                editor.revert(target)
            }
        }

        if case .label(let label) = target, !editor.keyTitle(of: target).isEmpty {
            Button(L10n.hotkeys.clear) {
                editor.setLetter("", forLabel: label)
            }
        }
    }
}
