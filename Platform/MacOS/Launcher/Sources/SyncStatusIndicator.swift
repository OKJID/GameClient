import SwiftUI

struct SyncStatusIndicator: View {
    let status: ApiSyncStatus
    let isLauncherOutdated: Bool

    @State private var isHovered = false

    private let dotSize: CGFloat = 9
    private let hintWidth: CGFloat = 300

    private var color: Color {
        guard !isLauncherOutdated else { return .red }

        switch status {
        case .initializing: return .gray
        case .current: return .green
        case .recent: return .yellow
        case .unverified: return .red
        }
    }

    private var hint: String {
        guard !isLauncherOutdated else { return L10n.sync.statusLauncherOutdated }

        switch status {
        case .initializing: return L10n.sync.statusChecking
        case .current: return L10n.sync.statusOk
        case .recent: return L10n.sync.statusStale
        case .unverified: return L10n.sync.statusOffline
        }
    }

    var body: some View {
        Circle()
            .fill(color)
            .frame(width: dotSize, height: dotSize)
            .shadow(color: color.opacity(0.8), radius: 4)
            .padding(4)
            .contentShape(Rectangle())
            .onHover { inside in isHovered = inside }
            .overlay(_buildHint(), alignment: .top)
    }

    @ViewBuilder
    private func _buildHint() -> some View {
        if isHovered {
            Text(hint)
                .font(.system(size: 11, weight: .medium, design: .monospaced))
                .foregroundColor(.white)
                .multilineTextAlignment(.leading)
                .fixedSize(horizontal: false, vertical: true)
                .padding(.horizontal, 10)
                .padding(.vertical, 6)
                .frame(width: hintWidth, alignment: .leading)
                .background(Color.black.opacity(0.9))
                .overlay(RoundedRectangle(cornerRadius: 6).stroke(color.opacity(0.6), lineWidth: 1))
                .clipShape(RoundedRectangle(cornerRadius: 6))
                .offset(y: dotSize + 16)
                .allowsHitTesting(false)
        }
    }
}
