import SwiftUI
import AppKit

struct HotkeysPageView: View {
    @ObservedObject var viewModel: LauncherViewModel

    var body: some View {
        if let files = viewModel.hotkeyFiles(for: viewModel.selectedProfile) {
            HotkeyEditorPage(viewModel: viewModel, files: files)
                .id(files.directory)
        } else {
            HotkeyUnavailablePage(viewModel: viewModel)
        }
    }
}

struct HotkeyEditorPage: View {
    @ObservedObject var viewModel: LauncherViewModel
    @StateObject private var editor: HotkeyEditorModel

    init(viewModel: LauncherViewModel, files: HotkeyFiles) {
        self.viewModel = viewModel
        _editor = StateObject(wrappedValue: HotkeyEditorModel(
            files: files,
            launch: viewModel.hotkeyEngineLaunch(for: viewModel.selectedProfile),
            isMapScrollOnWASD: viewModel.wasdMapScroll
        ))
    }

    @State private var isConfirmingReset = false

    private let pageInset: CGFloat = 32
    private let sectionListWidth: CGFloat = 260
    private let warning = Color(red: 1.0, green: 0.6, blue: 0.2)

    private var profile: GameProfile { viewModel.selectedProfile }
    private var accent: Color { profile.theme.accent }

    var body: some View {
        VStack(spacing: 0) {
            _buildHeader()
                .padding(.horizontal, pageInset)
                .padding(.top, 20)
                .padding(.bottom, 16)

            _buildBody()
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Color.black.opacity(0.6))
        .onAppear(perform: editor.refresh)
        .onDisappear(perform: editor.stopCapture)
        .alert(isPresented: $isConfirmingReset, content: _buildResetAlert)
    }

    private func _buildResetAlert() -> Alert {
        Alert(
            title: Text(L10n.hotkeys.resetTitle),
            message: Text(L10n.hotkeys.resetMsg),
            primaryButton: .destructive(Text(L10n.hotkeys.reset), action: editor.reset),
            secondaryButton: .cancel(Text(L10n.alerts.cancel))
        )
    }

    private func _buildHeader() -> some View {
        HStack(spacing: 16) {
            PageBackButton(accent: accent) { viewModel.route = .home }
                .disabled(editor.phase == .applying)

            Image(systemName: "keyboard")
                .foregroundColor(accent)
            Text(L10n.hotkeys.title)
                .font(.system(size: 18, weight: .black, design: .monospaced))
                .foregroundColor(.white)
            Text("\(profile.displayName) · \(viewModel.gameLanguage)".uppercased())
                .font(.system(size: 11, weight: .bold, design: .monospaced))
                .foregroundColor(.white.opacity(0.45))

            Spacer()

            _buildStatus()

            if editor.hasEdits || editor.hasOriginals {
                _buildHeaderButton(L10n.hotkeys.reset, systemImage: "arrow.counterclockwise", isProminent: false) {
                    isConfirmingReset = true
                }
            }

            if editor.hasEdits {
                _buildHeaderButton(L10n.hotkeys.apply, systemImage: "checkmark", isProminent: true, action: editor.apply)
            }
        }
    }

    @ViewBuilder
    private func _buildStatus() -> some View {
        if editor.dump != nil {
            switch editor.phase {
            case .idle:
                EmptyView()
            case .refreshing:
                _buildProgress(L10n.hotkeys.checking)
            case .applying:
                _buildProgress(L10n.hotkeys.applying)
            case .failed:
                _buildFailure()
            }
        }
    }

    private func _buildProgress(_ title: String) -> some View {
        HStack(spacing: 6) {
            ProgressView()
                .progressViewStyle(.circular)
                .controlSize(.small)
            Text(title)
                .font(.system(size: 10, design: .monospaced))
                .foregroundColor(.white.opacity(0.6))
        }
    }

    private func _buildFailure() -> some View {
        HStack(spacing: 8) {
            Image(systemName: "exclamationmark.triangle.fill")
                .font(.system(size: 11))
                .foregroundColor(warning)
            Text(L10n.hotkeys.failedShort)
                .font(.system(size: 10, design: .monospaced))
                .foregroundColor(warning)
            _buildHeaderButton(L10n.hotkeys.retry, systemImage: "arrow.clockwise", isProminent: false, action: editor.refresh)
        }
    }

    @ViewBuilder
    private func _buildBody() -> some View {
        if editor.dump == nil {
            _buildPlaceholder()
        } else {
            HStack(spacing: 0) {
                HotkeySectionList(editor: editor, accent: accent)
                    .frame(width: sectionListWidth)

                Rectangle()
                    .fill(Color.white.opacity(0.06))
                    .frame(width: 1)

                HotkeySectionContent(editor: editor, accent: accent)
            }
            .overlay(_buildApplyingVeil())
        }
    }

    private func _buildPlaceholder() -> some View {
        VStack(spacing: 14) {
            if editor.phase == .failed {
                Image(systemName: "exclamationmark.triangle")
                    .font(.system(size: 28))
                    .foregroundColor(warning)
                _buildPlaceholderText(L10n.hotkeys.failed)
                _buildHeaderButton(L10n.hotkeys.retry, systemImage: "arrow.clockwise", isProminent: true, action: editor.refresh)
            } else {
                ProgressView()
                    .progressViewStyle(.circular)
                _buildPlaceholderText(L10n.hotkeys.loading)
            }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }

    private func _buildPlaceholderText(_ text: String) -> some View {
        Text(text)
            .font(.system(size: 12, design: .monospaced))
            .foregroundColor(.white.opacity(0.7))
            .multilineTextAlignment(.center)
            .frame(maxWidth: 420)
    }

    @ViewBuilder
    private func _buildApplyingVeil() -> some View {
        if editor.phase == .applying {
            ZStack {
                Color.black.opacity(0.55)
                VStack(spacing: 12) {
                    ProgressView()
                        .progressViewStyle(.circular)
                    _buildPlaceholderText(L10n.hotkeys.applying)
                }
            }
        }
    }

    private func _buildHeaderButton(
        _ title: String,
        systemImage: String,
        isProminent: Bool,
        action: @escaping () -> Void
    ) -> some View {
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
        .disabled(editor.isBusy)
        .opacity(editor.isBusy ? 0.5 : 1)
        .onHover { inside in
            if inside { NSCursor.pointingHand.push() } else { NSCursor.pop() }
        }
    }
}

struct HotkeyUnavailablePage: View {
    @ObservedObject var viewModel: LauncherViewModel

    private let pageInset: CGFloat = 32

    var body: some View {
        VStack(spacing: 0) {
            HStack {
                PageBackButton(accent: viewModel.selectedProfile.theme.accent) { viewModel.route = .home }
                Spacer()
            }
            .padding(.horizontal, pageInset)
            .padding(.top, 20)

            Text(L10n.hotkeys.unavailable)
                .font(.system(size: 12, design: .monospaced))
                .foregroundColor(.white.opacity(0.7))
                .multilineTextAlignment(.center)
                .frame(maxWidth: .infinity, maxHeight: .infinity)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Color.black.opacity(0.6))
    }
}
