import SwiftUI

struct OwnModPanel<LaunchRow: View>: View {
    @ObservedObject private var controller: OwnModController
    private let theme: LauncherTheme
    private let onHowItWorks: () -> Void
    private let launchRow: LaunchRow

    init(
        controller: OwnModController,
        theme: LauncherTheme,
        onHowItWorks: @escaping () -> Void,
        @ViewBuilder launchRow: () -> LaunchRow
    ) {
        _controller = ObservedObject(wrappedValue: controller)
        self.theme = theme
        self.onHowItWorks = onHowItWorks
        self.launchRow = launchRow()
    }

    private let pathMaxWidth: CGFloat = 460
    private let bannerOpacity: Double = 0.40
    private let bannerBlur: CGFloat = 1

    var body: some View {
        _buildContent()
            .padding(.horizontal, 30)
            .padding(.vertical, 12)
            .background(_buildBackground())
            .clipShape(RoundedRectangle(cornerRadius: 8))
            .overlay(RoundedRectangle(cornerRadius: 8).stroke(theme.accent.opacity(0.35), lineWidth: 1))
    }

    private func _buildBackground() -> some View {
        ZStack {
            theme.panel.opacity(0.55)

            if let banner = controller.banner {
                Image(nsImage: banner)
                    .resizable()
                    .aspectRatio(contentMode: .fill)
                    .blur(radius: bannerBlur)
                    .opacity(bannerOpacity)

                RadialGradient(
                    colors: [.clear, .black.opacity(0.45)],
                    center: .center,
                    startRadius: 60,
                    endRadius: 360
                )
            }
        }
    }

    @ViewBuilder
    private func _buildContent() -> some View {
        if let spec = controller.state.spec {
            _buildSelected(spec)
        } else {
            _buildEmpty()
        }
    }

    private func _buildEmpty() -> some View {
        HStack(spacing: 12) {
            _buildLargeButton(title: OwnModText.howItWorks, icon: "book", isPrimary: false, action: onHowItWorks)
            _buildLargeButton(title: OwnModText.chooseFolder, icon: "folder.badge.plus", isPrimary: true, action: controller.chooseFolder)
        }
    }

    private func _buildSelected(_ spec: OwnModSpec) -> some View {
        VStack(spacing: 8) {
            _buildFolderPath(spec.folder)

            ForEach(controller.state.issues) { issue in
                _buildIssue(issue)
            }

            launchRow

            HStack(spacing: 12) {
                _buildSmallButton(title: OwnModText.howItWorks, icon: "book", action: onHowItWorks)
                _buildSmallButton(title: OwnModText.changeFolder, icon: "folder", action: controller.chooseFolder)
            }
        }
    }

    private func _buildFolderPath(_ folder: URL) -> some View {
        HStack(spacing: 8) {
            Image(systemName: "folder.fill")
                .foregroundColor(theme.accentSoft)

            Text(folder.path)
                .font(.system(size: 12, weight: .bold, design: .monospaced))
                .foregroundColor(.white.opacity(0.75))
                .lineLimit(1)
                .truncationMode(.middle)
                .textSelection(.enabled)
                .help(folder.path)
        }
        .frame(maxWidth: pathMaxWidth)
    }

    private func _buildIssue(_ issue: OwnModIssue) -> some View {
        HStack(spacing: 6) {
            Image(systemName: "exclamationmark.triangle.fill")

            Text(issue.message)
                .lineLimit(2)
                .multilineTextAlignment(.center)
        }
        .font(.system(size: 11, weight: .medium, design: .monospaced))
        .foregroundColor(issue.severity == .blocking ? .red.opacity(0.85) : .orange.opacity(0.85))
        .frame(maxWidth: pathMaxWidth)
    }

    private func _buildLargeButton(
        title: String,
        icon: String,
        isPrimary: Bool,
        action: @escaping () -> Void
    ) -> some View {
        Button(action: action) {
            HStack(spacing: 10) {
                Image(systemName: icon)
                Text(title)
            }
            .font(.system(size: 18, weight: .bold, design: .monospaced))
            .foregroundColor(.white)
            .padding(.horizontal, 28)
            .padding(.vertical, 12)
            .background(theme.accent.opacity(isPrimary ? 0.30 : 0.10))
            .overlay(RoundedRectangle(cornerRadius: 8).stroke(theme.accent.opacity(isPrimary ? 1.0 : 0.5), lineWidth: 2))
            .clipShape(RoundedRectangle(cornerRadius: 8))
        }
        .buttonStyle(PlainButtonStyle())
    }

    private func _buildSmallButton(title: String, icon: String, action: @escaping () -> Void) -> some View {
        Button(action: action) {
            HStack(spacing: 5) {
                Image(systemName: icon)
                Text(title)
            }
            .font(.system(size: 11, weight: .bold, design: .monospaced))
            .foregroundColor(theme.accentSoft)
            .padding(.horizontal, 14)
            .padding(.vertical, 6)
            .background(RoundedRectangle(cornerRadius: 4).fill(theme.accentSoft.opacity(0.12)))
            .overlay(RoundedRectangle(cornerRadius: 4).stroke(theme.accentSoft.opacity(0.6), lineWidth: 1))
        }
        .buttonStyle(PlainButtonStyle())
    }
}
