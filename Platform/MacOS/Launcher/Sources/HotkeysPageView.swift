import SwiftUI
import AppKit
import UniformTypeIdentifiers

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
            presetsFolder: viewModel.selectedProfile.hotkeyPresetsDirURL,
            installRoot: viewModel.hotkeyInstallRoot(for: viewModel.selectedProfile)?.path ?? "",
            launch: viewModel.hotkeyEngineLaunch(for: viewModel.selectedProfile),
            isMapScrollOnWASD: viewModel.wasdMapScroll
        ))
    }

    @State private var isConfirmingReset = false
    @State private var isPresetPickerOpen = false

    private let pageInset: CGFloat = 32
    private let sectionListWidth: CGFloat = 260
    private let presetPickerGap: CGFloat = 6
    private let warning = Color(red: 1.0, green: 0.6, blue: 0.2)

    private var profile: GameProfile { viewModel.selectedProfile }
    private var accent: Color { profile.theme.accent }

    private var presetTitle: String {
        let name = editor.presetName ?? L10n.hotkeys.presetStandard
        return editor.isPresetModified ? "\(name)*" : name
    }

    private func _togglePresetPicker() {
        isPresetPickerOpen.toggle()
    }

    private func _closePresetPicker(then action: () -> Void) {
        isPresetPickerOpen = false
        action()
    }

    private func _selectStandardPreset() {
        isPresetPickerOpen = false
        guard editor.hasOriginals else {
            editor.discardEdits()
            return
        }

        isConfirmingReset = true
    }

    private func _requestRestoreOriginal() {
        isConfirmingReset = true
    }

    private func _deletePreset(_ entry: HotkeyPresetEntry) {
        let isCurrent = entry.name == editor.presetName
        guard editor.deleteOwnPreset(entry), isCurrent else { return }

        _selectStandardPreset()
    }

    private var ownPresetTypes: [UTType] {
        HotkeyPresetStore.ownExtensions.compactMap { UTType(filenameExtension: $0) }
    }

    private func _chooseImportFile() {
        let panel = NSOpenPanel()
        panel.canChooseFiles = true
        panel.canChooseDirectories = false
        panel.allowsMultipleSelection = false
        panel.allowedContentTypes = ownPresetTypes
        panel.message = L10n.hotkeys.presetImportMessage

        guard panel.runModal() == .OK, let url = panel.url else { return }
        editor.importPreset(from: url)
    }

    private func _chooseExportDestination(fileExtension: String) {
        guard let type = UTType(filenameExtension: fileExtension) else { return }

        let panel = NSSavePanel()
        panel.allowedContentTypes = [type]
        panel.canCreateDirectories = true
        panel.directoryURL = _ensurePresetsFolder()
        panel.nameFieldStringValue = L10n.hotkeys.presetDefaultName
        panel.message = L10n.hotkeys.presetSaveNote

        guard panel.runModal() == .OK, let url = panel.url else { return }
        editor.exportApplied(to: url)
    }

    private func _openPresetFolder() {
        NSWorkspace.shared.open(_ensurePresetsFolder())
    }

    private func _ensurePresetsFolder() -> URL {
        try? FileManager.default.createDirectory(at: editor.presetsFolder, withIntermediateDirectories: true)
        return editor.presetsFolder
    }

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
        .overlay(_buildPresetDismissLayer())
        .overlayPreferenceValue(HotkeyPresetAnchorKey.self, _buildPresetDropdown)
        .onAppear(perform: editor.refresh)
        .onDisappear(perform: editor.stopCapture)
        .onReceive(viewModel.hotkeyPresetInstaller.$installedRevision) { _ in editor.reloadPresets() }
        .overlay(_buildResetConfirmation())
    }

    private func _confirmReset() {
        isConfirmingReset = false
        editor.reset()
    }

    private func _cancelReset() {
        isConfirmingReset = false
    }

    @ViewBuilder
    private func _buildResetConfirmation() -> some View {
        if isConfirmingReset {
            HotkeyConfirmDialog(
                title: L10n.hotkeys.resetTitle,
                message: L10n.hotkeys.resetMsg,
                systemImage: "arrow.counterclockwise",
                confirmTitle: L10n.hotkeys.restoreOriginal,
                cancelTitle: L10n.alerts.cancel,
                accent: accent,
                onConfirm: _confirmReset,
                onCancel: _cancelReset
            )
        }
    }

    @ViewBuilder
    private func _buildPresetDismissLayer() -> some View {
        if isPresetPickerOpen {
            Color.white.opacity(0.001)
                .onTapGesture { isPresetPickerOpen = false }
        }
    }

    @ViewBuilder
    private func _buildPresetDropdown(_ anchor: Anchor<CGRect>?) -> some View {
        if isPresetPickerOpen, let anchor {
            GeometryReader { proxy in
                HotkeyPresetDropdown(
                    accent: accent,
                    currentName: editor.presetName,
                    shippedPresets: editor.shippedPresets,
                    ownPresets: editor.ownPresets,
                    onStandard: _selectStandardPreset,
                    onPreset: { entry in _closePresetPicker { editor.applyPreset(entry) } },
                    onDelete: _deletePreset,
                    onImport: { _closePresetPicker(then: _chooseImportFile) },
                    onExport: { fileExtension in _closePresetPicker { _chooseExportDestination(fileExtension: fileExtension) } },
                    onOpenFolder: { _closePresetPicker(then: _openPresetFolder) }
                )
                .offset(x: proxy[anchor].minX, y: proxy[anchor].maxY + presetPickerGap)
            }
        }
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

            if editor.dump != nil {
                HotkeyPresetSelector(
                    title: presetTitle,
                    accent: accent,
                    isOpen: isPresetPickerOpen,
                    isEnabled: !editor.isBusy,
                    action: _togglePresetPicker
                )
            }

            Spacer()

            _buildStatus()

            _buildHeaderActions()
        }
    }

    @ViewBuilder
    private func _buildHeaderActions() -> some View {
        if editor.hasEdits {
            _buildHeaderButton(L10n.alerts.cancel, systemImage: "xmark", isProminent: false, action: editor.discardEdits)
            _buildHeaderButton(L10n.hotkeys.apply, systemImage: "checkmark", isProminent: true, action: editor.apply)
        } else if editor.hasOriginals {
            _buildHeaderButton(L10n.hotkeys.restoreOriginal, systemImage: "arrow.counterclockwise", isProminent: false, action: _requestRestoreOriginal)
        }
    }

    @ViewBuilder
    private func _buildStatus() -> some View {
        if editor.dump != nil {
            switch editor.phase {
            case .idle:
                _buildPresetOutcome()
            case .refreshing:
                _buildProgress(L10n.hotkeys.checking)
            case .applying:
                _buildProgress(L10n.hotkeys.applying)
            case .importing:
                _buildProgress(L10n.hotkeys.importing)
            case .exporting:
                _buildProgress(L10n.hotkeys.saving)
            case .failed:
                _buildFailure()
            }
        }
    }

    @ViewBuilder
    private func _buildPresetOutcome() -> some View {
        switch editor.presetOutcome {
        case .saved(let fileName):
            _buildOutcomeText(String(format: L10n.hotkeys.presetSaved, fileName), color: .white.opacity(0.6))
        case .saveFailed(let fileName):
            _buildOutcomeText(String(format: L10n.hotkeys.presetSaveFailed, fileName), color: warning)
        case .importFailed(let fileName):
            _buildOutcomeText(String(format: L10n.hotkeys.presetImportFailed, fileName), color: warning)
        case .deleteFailed(let fileName):
            _buildOutcomeText(String(format: L10n.hotkeys.presetDeleteFailed, fileName), color: warning)
        case nil:
            EmptyView()
        }
    }

    private func _buildOutcomeText(_ text: String, color: Color) -> some View {
        Text(text)
            .font(.system(size: 10, design: .monospaced))
            .foregroundColor(color)
            .lineLimit(1)
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
