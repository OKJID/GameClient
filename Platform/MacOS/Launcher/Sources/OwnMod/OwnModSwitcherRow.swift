import SwiftUI

struct OwnModSwitcherRow: View {
    let isSelected: Bool
    let isReady: Bool
    let banner: NSImage?
    let width: CGFloat
    let height: CGFloat
    let onSelect: () -> Void

    private let theme = LauncherTheme.ownMod
    private let bannerOpacity: Double = 0.55

    private var foreground: Color { isSelected ? .white : theme.accentSoft }

    var body: some View {
        Button(action: onSelect) {
            HStack(spacing: 0) {
                Rectangle()
                    .fill(theme.accent)
                    .frame(width: 5)

                Image(systemName: "folder.fill")
                    .font(.system(size: 10, weight: .bold))
                    .foregroundColor(foreground)
                    .padding(.leading, 14)

                Text(OwnModText.rowTitle)
                    .font(.system(size: 10, weight: .bold, design: .monospaced))
                    .foregroundColor(foreground)
                    .lineLimit(1)
                    .minimumScaleFactor(0.7)
                    .padding(.leading, 6)

                Spacer(minLength: 0)

                _buildTrailingMark()
            }
            .frame(width: width, height: height)
            .background(_buildBackground())
            .overlay(_buildBorder())
        }
        .buttonStyle(PlainButtonStyle())
        .opacity(isReady ? 1.0 : 0.65)
        .help(OwnModText.displayName)
    }

    @ViewBuilder
    private func _buildTrailingMark() -> some View {
        if isSelected {
            ChevronMark(thickness: 6)
                .fill(theme.accentSoft)
                .frame(width: 11, height: 20)
                .padding(.trailing, 12)
        } else if isReady {
            Image(systemName: "checkmark.circle.fill")
                .font(.system(size: 10))
                .foregroundColor(theme.accentSoft)
                .padding(.trailing, 12)
        }
    }

    private func _buildBackground() -> some View {
        ZStack {
            LeftFlushShape(radius: 10)
                .fill(isSelected ? theme.accent.opacity(0.85) : theme.panel.opacity(0.75))

            if let banner {
                _buildBanner(banner)
            }
        }
    }

    private func _buildBanner(_ banner: NSImage) -> some View {
        ZStack {
            Image(nsImage: banner)
                .resizable()
                .aspectRatio(contentMode: .fill)
                .opacity(bannerOpacity)

            LinearGradient(
                stops: [
                    .init(color: .black.opacity(0.6), location: 0),
                    .init(color: .clear, location: 0.7)
                ],
                startPoint: .leading,
                endPoint: .trailing
            )
        }
        .frame(width: width, height: height)
        .clipShape(LeftFlushShape(radius: 10))
    }

    private func _buildBorder() -> some View {
        LeftFlushShape(radius: 10)
            .stroke(isSelected ? theme.accentSoft : theme.panelBorder, lineWidth: isSelected ? 2 : 1)
    }
}
