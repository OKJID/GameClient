import SwiftUI
import AppKit

struct HotkeyConfirmDialog: View {
    let title: String
    let message: String
    let systemImage: String
    let confirmTitle: String
    let cancelTitle: String
    let accent: Color
    let onConfirm: () -> Void
    let onCancel: () -> Void

    private let width: CGFloat = 440
    private let panelColor = Color(red: 0.07, green: 0.08, blue: 0.1)

    var body: some View {
        ZStack {
            Color.black.opacity(0.55)
                .contentShape(Rectangle())
                .onTapGesture(perform: onCancel)

            _buildPanel()
        }
    }

    private func _buildPanel() -> some View {
        VStack(alignment: .leading, spacing: 14) {
            _buildTitle()
            Text(message)
                .font(.system(size: 11, design: .monospaced))
                .foregroundColor(.white.opacity(0.7))
                .lineSpacing(2)
                .fixedSize(horizontal: false, vertical: true)
            _buildActions()
        }
        .padding(20)
        .frame(width: width, alignment: .leading)
        .background(RoundedRectangle(cornerRadius: 8).fill(panelColor.opacity(0.98)))
        .overlay(
            RoundedRectangle(cornerRadius: 8)
                .stroke(accent.opacity(0.45), lineWidth: 1)
        )
        .shadow(color: .black.opacity(0.6), radius: 18, y: 8)
    }

    private func _buildTitle() -> some View {
        HStack(alignment: .top, spacing: 10) {
            Image(systemName: systemImage)
                .font(.system(size: 14, weight: .bold))
                .foregroundColor(accent)
            Text(title)
                .font(.system(size: 14, weight: .black, design: .monospaced))
                .foregroundColor(.white)
                .fixedSize(horizontal: false, vertical: true)
        }
    }

    private func _buildActions() -> some View {
        HStack(spacing: 10) {
            Spacer()
            HotkeyDialogButton(title: cancelTitle, systemImage: "xmark", isProminent: false, accent: accent, action: onCancel)
            HotkeyDialogButton(title: confirmTitle, systemImage: systemImage, isProminent: true, accent: accent, action: onConfirm)
        }
        .padding(.top, 4)
    }
}

private struct HotkeyDialogButton: View {
    let title: String
    let systemImage: String
    let isProminent: Bool
    let accent: Color
    let action: () -> Void

    var body: some View {
        Button(action: action) {
            HStack(spacing: 6) {
                Image(systemName: systemImage)
                    .font(.system(size: 10, weight: .bold))
                Text(title.uppercased())
                    .font(.system(size: 11, weight: .black, design: .monospaced))
            }
            .foregroundColor(isProminent ? .black : accent)
            .padding(.horizontal, 12)
            .padding(.vertical, 7)
            .background(RoundedRectangle(cornerRadius: 6).fill(isProminent ? accent : Color.clear))
            .overlay(
                RoundedRectangle(cornerRadius: 6)
                    .stroke(accent.opacity(0.45), lineWidth: 1)
            )
            .contentShape(RoundedRectangle(cornerRadius: 6))
        }
        .buttonStyle(PlainButtonStyle())
        .onHover { inside in
            if inside { NSCursor.pointingHand.push() } else { NSCursor.pop() }
        }
    }
}
