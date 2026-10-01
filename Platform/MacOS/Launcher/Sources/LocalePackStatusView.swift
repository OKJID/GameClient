import SwiftUI
import AppKit

extension LocalePackStatus {
    func label(for profile: GameProfile) -> String? {
        switch self {
        case .hidden:
            return nil
        case .native:
            return L10n.settings.localeNative
        case .installed(let pack):
            return pack.hasVoices(for: profile) ? L10n.settings.localeVoicesInstalled : L10n.settings.localeExtrasInstalled
        case .outdated(let pack):
            return String(format: L10n.settings.localeUpdate, pack.downloadSizeMB)
        case .available(let pack):
            let format = pack.hasVoices(for: profile) ? L10n.settings.localeDownloadVoices : L10n.settings.localeDownloadExtras
            return String(format: format, pack.downloadSizeMB)
        case .running(let state):
            return state.statusText
        case .failed(let message, _):
            return String(format: L10n.mod.status.error, message)
        }
    }
}

struct LocalePackStatusView: View {
    @ObservedObject var viewModel: LauncherViewModel
    @Binding var isHovered: Bool

    private let neonGreen = Color(red: 0.1, green: 0.9, blue: 0.4)

    private var accent: Color { viewModel.selectedProfile.theme.accent }
    private var status: LocalePackStatus { viewModel.localePackStatus }
    private var label: String { status.label(for: viewModel.selectedProfile) ?? "" }

    var body: some View {
        _buildContent()
            .onHover { inside in isHovered = inside }
    }

    @ViewBuilder
    private func _buildContent() -> some View {
        switch status {
        case .hidden:
            EmptyView()
        case .native:
            _buildBadge()
        case .installed:
            HStack(spacing: 8) {
                _buildBadge()
                _buildRemoveButton()
            }
        case .outdated, .available:
            _buildActionButton(label)
        case .running(let state):
            _buildProgress(state)
        case .failed:
            _buildFailure()
        }
    }

    private func _buildBadge() -> some View {
        HStack(spacing: 6) {
            Image(systemName: "checkmark.seal.fill")
                .font(.system(size: 12))
                .foregroundColor(neonGreen)

            Text(label)
                .font(.system(size: 11, weight: .medium, design: .monospaced))
                .foregroundColor(.white.opacity(0.8))
                .lineLimit(1)
        }
    }

    private func _buildActionButton(_ title: String) -> some View {
        Button(action: { viewModel.installLocalePack() }) {
            HStack(spacing: 6) {
                Image(systemName: "arrow.down.circle")
                    .font(.system(size: 12))

                Text(title)
                    .font(.system(size: 11, weight: .bold, design: .monospaced))
                    .lineLimit(1)
            }
            .foregroundColor(.white)
            .padding(.horizontal, 10)
            .padding(.vertical, 5)
            .background(accent.opacity(0.18))
            .clipShape(RoundedRectangle(cornerRadius: 4))
            .overlay(
                RoundedRectangle(cornerRadius: 4)
                    .stroke(accent.opacity(0.6), lineWidth: 1)
            )
        }
        .buttonStyle(PlainButtonStyle())
        .onHover { inside in
            if inside { NSCursor.pointingHand.push() } else { NSCursor.pop() }
        }
    }

    private func _buildRemoveButton() -> some View {
        Button(action: { viewModel.isConfirmingLocaleRemoval = true }) {
            Image(systemName: "trash")
                .font(.system(size: 11))
                .foregroundColor(.white.opacity(0.5))
        }
        .buttonStyle(PlainButtonStyle())
        .onHover { inside in
            if inside { NSCursor.pointingHand.push() } else { NSCursor.pop() }
        }
    }

    private func _buildProgress(_ state: ModInstallState) -> some View {
        HStack(spacing: 8) {
            ProgressView(value: state.fraction)
                .progressViewStyle(.linear)
                .tint(accent)
                .frame(width: 90)

            Text(label)
                .font(.system(size: 10, design: .monospaced))
                .foregroundColor(.white.opacity(0.7))
                .lineLimit(1)
        }
    }

    private func _buildFailure() -> some View {
        HStack(spacing: 8) {
            Text(label)
                .font(.system(size: 10, design: .monospaced))
                .foregroundColor(.red.opacity(0.85))
                .lineLimit(1)
                .truncationMode(.tail)

            _buildActionButton(L10n.settings.localeRetry)
        }
    }
}

struct LocalePackHint: View {
    let text: String
    let accent: Color

    private let gap: CGFloat = 6

    var body: some View {
        GeometryReader { geometry in
            Text(text)
                .font(.system(size: 11, weight: .medium, design: .monospaced))
                .foregroundColor(.white)
                .multilineTextAlignment(.leading)
                .fixedSize(horizontal: false, vertical: true)
                .padding(.horizontal, 10)
                .padding(.vertical, 6)
                .frame(width: geometry.size.width, alignment: .leading)
                .background(Color.black.opacity(0.9))
                .overlay(RoundedRectangle(cornerRadius: 6).stroke(accent.opacity(0.6), lineWidth: 1))
                .clipShape(RoundedRectangle(cornerRadius: 6))
                .offset(y: geometry.size.height + gap)
        }
        .allowsHitTesting(false)
    }
}
