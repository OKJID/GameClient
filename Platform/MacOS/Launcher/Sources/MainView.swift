import SwiftUI
import AppKit
import Combine

// MARK: - Main View

struct LeftFlushShape: Shape {
    let radius: CGFloat

    func path(in rect: CGRect) -> Path {
        var path = Path()
        path.move(to: CGPoint(x: rect.minX, y: rect.minY))
        path.addLine(to: CGPoint(x: rect.maxX - radius, y: rect.minY))
        path.addQuadCurve(
            to: CGPoint(x: rect.maxX, y: rect.minY + radius),
            control: CGPoint(x: rect.maxX, y: rect.minY)
        )
        path.addLine(to: CGPoint(x: rect.maxX, y: rect.maxY - radius))
        path.addQuadCurve(
            to: CGPoint(x: rect.maxX - radius, y: rect.maxY),
            control: CGPoint(x: rect.maxX, y: rect.maxY)
        )
        path.addLine(to: CGPoint(x: rect.minX, y: rect.maxY))
        path.closeSubpath()
        return path
    }
}

struct ChevronMark: Shape {
    let thickness: CGFloat

    func path(in rect: CGRect) -> Path {
        var path = Path()
        path.move(to: CGPoint(x: rect.minX, y: rect.minY))
        path.addLine(to: CGPoint(x: rect.maxX, y: rect.midY))
        path.addLine(to: CGPoint(x: rect.minX, y: rect.maxY))
        path.addLine(to: CGPoint(x: rect.minX + thickness, y: rect.maxY))
        path.addLine(to: CGPoint(x: rect.maxX + thickness, y: rect.midY))
        path.addLine(to: CGPoint(x: rect.minX + thickness, y: rect.minY))
        path.closeSubpath()
        return path
    }
}

struct MainView: View {
    @StateObject private var viewModel = LauncherViewModel()

    private var theme: LauncherTheme { viewModel.selectedProfile.theme }
    private var accent: Color { theme.accent }
    private let switcherButtonWidth: CGFloat = 114
    private let switcherContentGap: CGFloat = 12
    private let contentInset: CGFloat = 60

    private var switcherColumnWidth: CGFloat {
        switcherButtonWidth + switcherContentGap - contentInset
    }
    private let neonGreen = Color(red: 0.1, green: 0.9, blue: 0.4)
    private let darkPanel = Color.black.opacity(0.85)
    private let steamGradient = LinearGradient(
        colors: [Color(red: 0.06, green: 0.12, blue: 0.24), Color(red: 0.02, green: 0.06, blue: 0.14)],
        startPoint: .top,
        endPoint: .bottom
    )

    var body: some View {
        GeometryReader { geometry in
            ZStack {
                WindowAccessor().frame(width: 0, height: 0)

                _buildBackground(size: geometry.size)
                
                _buildFooter()
                    .padding(.top, 72)
                    .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)

                HStack(spacing: 0) {
                    _buildGameSwitcher()

                    VStack(spacing: 0) {
                        _buildHeader()
                            .padding(.top, 16)

                        if let update = viewModel.updateChecker.availableUpdate, !viewModel.isUpdateDismissed {
                            _buildUpdateBanner(update)
                                .padding(.horizontal, 40)
                                .padding(.top, 12)
                        }

                        _buildTabBar()
                            .padding(.top, 20)

                        _buildActiveTab()
                            .padding(.horizontal, 40)
                            .padding(.top, 8)

                        Spacer()

                        _buildBottomAction()
                            .padding(.bottom, 8)
                        AnnouncementPortalView(items: viewModel.announcements.items, accent: accent)
                            .padding(.horizontal, 40)
                            .padding(.bottom, 8)
                    }
                    .padding(.horizontal, 20)
                    .frame(maxWidth: .infinity)

                    SidebarView(viewModel: viewModel)
                }
            }
            .frame(width: geometry.size.width, height: geometry.size.height)
            .edgesIgnoringSafeArea(.all)
        }
        .alert(item: _activeAlert) { alert in
            _buildAlert(alert)
        }
    }

    private var _activeAlert: Binding<LauncherAlert?> {
        Binding(
            get: {
                if let message = viewModel.alertMessage {
                    return .message(message)
                }

                if viewModel.steamCMD.showPurchaseAlert {
                    return .purchase(username: viewModel.steamCMD.lastUsername)
                }

                if viewModel.steamCMD.showRosettaAlert {
                    return .rosetta
                }

                if viewModel.showPatchConfirmation {
                    return .patchConfirmation
                }

                if let confirmation = viewModel.modConfirmation {
                    return .modConfirmation(confirmation)
                }

                if viewModel.isAskingAboutStoredPassword, let account = viewModel.storedPasswordAccount {
                    return .storedPassword(account: account)
                }

                return nil
            },
            set: { _ in
                viewModel.alertMessage = nil
                viewModel.steamCMD.showPurchaseAlert = false
                viewModel.steamCMD.showRosettaAlert = false
                viewModel.showPatchConfirmation = false
                viewModel.modConfirmation = nil
                viewModel.isAskingAboutStoredPassword = false
            }
        )
    }

    private func _buildAlert(_ alert: LauncherAlert) -> Alert {
        switch alert {
        case .message(let text):
            return Alert(
                title: Text(L10n.alerts.launchError),
                message: Text(text),
                dismissButton: .default(Text(L10n.alerts.ok))
            )

        case .purchase(let username):
            return Alert(
                title: Text(L10n.alerts.gameNotFound),
                message: Text(L10n.alerts.gameNotFoundMsg.replacingOccurrences(of: "%@", with: username)),
                primaryButton: .default(Text(L10n.alerts.openSteamStore)) {
                    Analytics.logLinkOpened(target: "steam_store", location: "purchase_alert")

                    if let url = URL(string: SteamCMDManager.storeURL) {
                        NSWorkspace.shared.open(url)
                    }
                },
                secondaryButton: .cancel(Text(L10n.alerts.close))
            )

        case .rosetta:
            return Alert(
                title: Text(L10n.alerts.rosettaTitle),
                message: Text(L10n.alerts.rosettaMsg),
                primaryButton: .default(Text(L10n.alerts.rosettaInstall)) {
                    viewModel.steamCMD.installRosetta()
                },
                secondaryButton: .cancel(Text(L10n.alerts.cancel)) {
                    viewModel.steamCMD.declineRosetta()
                }
            )

        case .patchConfirmation:
            return Alert(
                title: Text(L10n.alerts.patchTitle),
                message: Text(L10n.alerts.patchMsg),
                primaryButton: .destructive(Text(L10n.alerts.patchButton)) {
                    viewModel.confirmPatching()
                },
                secondaryButton: .cancel(Text(L10n.alerts.cancel))
            )

        case .modConfirmation(let confirmation):
            return _buildModConfirmationAlert(confirmation)

        case .storedPassword(let account):
            return Alert(
                title: Text(L10n.steam.storedPasswordTitle),
                message: Text(String(format: L10n.steam.storedPasswordMsg, account)),
                primaryButton: .default(Text(L10n.steam.storedPasswordUse)) {
                    viewModel.revealStoredPassword()
                },
                secondaryButton: .cancel(Text(L10n.steam.storedPasswordType)) {
                    viewModel.dismissStoredPassword()
                }
            )
        }
    }

    private func _buildModConfirmationAlert(_ confirmation: LauncherViewModel.ModConfirmation) -> Alert {
        let profile = confirmation.profile
        let isRemoval = confirmation.kind == .remove

        let title = String(
            format: isRemoval ? L10n.mod.confirmRemoveTitle : L10n.mod.confirmReinstallTitle,
            profile.displayName
        )

        let message = isRemoval
            ? L10n.mod.confirmRemoveMsg
            : String(format: L10n.mod.confirmReinstallMsg, _modDownloadSizeText(profile))

        return Alert(
            title: Text(title),
            message: Text(message),
            primaryButton: .destructive(Text(isRemoval ? L10n.mod.remove : L10n.mod.reinstall)) {
                viewModel.confirmModAction(confirmation)
            },
            secondaryButton: .cancel(Text(L10n.alerts.cancel))
        )
    }

    // MARK: - Background

    @ViewBuilder
    private func _buildBackground(size: CGSize) -> some View {
        if let nsImage = _loadBackgroundImage() {
            Image(nsImage: nsImage)
                .resizable()
                .scaledToFill()
                .frame(width: size.width, height: size.height)
                .clipped()
                .overlay(Color.black.opacity(0.55))
        }
    }

    private func _loadBackgroundImage() -> NSImage? {
        let names = [theme.backgroundImageName, "background"]

        for name in names {
            guard let path = Bundle.main.path(forResource: name, ofType: "png"),
                  let image = NSImage(contentsOfFile: path) else {
                continue
            }

            return image
        }

        return nil
    }

    // MARK: - Header

    private func _buildGameSwitcher() -> some View {
        VStack(alignment: .leading, spacing: 8) {
            ForEach(GameProfile.baseGames) { profile in
                _buildGameButton(profile, isEnabled: viewModel.installDirectory(for: profile) != nil)
            }

            _buildModSwitcherSection()

            Spacer()
        }
        .padding(.top, 150)
        .frame(width: switcherColumnWidth, alignment: .leading)
    }

    @ViewBuilder
    private func _buildModSwitcherSection() -> some View {
        let mods = viewModel.availableMods

        if !mods.isEmpty {
            Text(L10n.mod.section)
                .font(.system(size: 9, weight: .bold, design: .monospaced))
                .foregroundColor(.white.opacity(0.35))
                .padding(.leading, 8)
                .padding(.top, 10)

            ForEach(mods) { profile in
                _buildGameButton(profile, isEnabled: true)
            }
        }
    }

    private func _buildGameButton(_ profile: GameProfile, isEnabled: Bool) -> some View {
        let isSelected = viewModel.selectedGameID == profile.id
        let isInstalledMod = profile.isMod && viewModel.isModInstalled(profile)

        return Button(action: { viewModel.selectGame(profile.id) }) {
            HStack(spacing: 0) {
                Rectangle()
                    .fill(profile.theme.accent)
                    .frame(width: 5)

                Text(profile.shortName)
                    .font(.system(size: profile.isMod ? 10 : 12, weight: .bold, design: .monospaced))
                    .foregroundColor(isSelected ? .white : profile.theme.accentSoft)
                    .lineLimit(1)
                    .padding(.leading, 14)

                Spacer(minLength: 0)

                if isInstalledMod, !isSelected {
                    Image(systemName: "checkmark.circle.fill")
                        .font(.system(size: 10))
                        .foregroundColor(profile.theme.accentSoft)
                        .padding(.trailing, 12)
                }

                if isSelected {
                    ChevronMark(thickness: 6)
                        .fill(profile.theme.accentSoft)
                        .frame(width: 11, height: 20)
                        .padding(.trailing, 12)
                }
            }
            .frame(width: switcherButtonWidth, height: 46)
            .background(
                LeftFlushShape(radius: 10)
                    .fill(isSelected ? profile.theme.accent.opacity(0.85) : profile.theme.panel.opacity(0.75))
            )
            .overlay(
                LeftFlushShape(radius: 10)
                    .stroke(isSelected ? profile.theme.accentSoft : profile.theme.panelBorder, lineWidth: isSelected ? 2 : 1)
            )
        }
        .buttonStyle(PlainButtonStyle())
        .disabled(!isEnabled)
        .opacity(_switcherOpacity(isEnabled: isEnabled, isInstalledMod: isInstalledMod, isMod: profile.isMod))
        .help(_switcherHelp(profile, isEnabled: isEnabled, isInstalledMod: isInstalledMod))
    }

    private func _switcherOpacity(isEnabled: Bool, isInstalledMod: Bool, isMod: Bool) -> Double {
        guard isEnabled else { return 0.35 }
        return isMod && !isInstalledMod ? 0.65 : 1.0
    }

    private func _switcherHelp(_ profile: GameProfile, isEnabled: Bool, isInstalledMod: Bool) -> String {
        guard isEnabled else { return "\(profile.displayName) — not installed" }
        guard profile.isMod else { return profile.displayName }

        return isInstalledMod
            ? "\(profile.displayName) — installed in \(viewModel.activeTab.rawValue)"
            : "\(profile.displayName) — not installed in \(viewModel.activeTab.rawValue)"
    }

    private func _buildHeader() -> some View {
        VStack(spacing: 4) {
            Text(L10n.app.title)
                .font(.system(size: 48, weight: .black))
                .foregroundColor(.white)
                .shadow(color: accent, radius: 15)

            Text(L10n.app.subtitle)
                .font(.system(size: 14, weight: .bold, design: .monospaced))
                .foregroundColor(.white.opacity(0.7))
                .shadow(color: .black, radius: 2, x: 0, y: 2)
        }
    }

    // MARK: - Tab Bar

    private func _buildTabBar() -> some View {
        HStack(spacing: 0) {
            ForEach(LauncherViewModel.Tab.allCases, id: \.self) { tab in
                _buildTabButton(tab)
            }
        }
        .background(Color.black.opacity(0.5))
        .clipShape(RoundedRectangle(cornerRadius: 6))
        .overlay(RoundedRectangle(cornerRadius: 6).stroke(accent.opacity(0.3), lineWidth: 1))
        .padding(.horizontal, 40)
    }

    private func _buildTabButton(_ tab: LauncherViewModel.Tab) -> some View {
        let isActive = viewModel.activeTab == tab
        let label: String
        let icon: String

        switch tab {
        case .steam:
            label = L10n.tab.steam
            icon = "arrow.down.circle.fill"
        case .local:
            label = L10n.tab.local
            icon = "folder.fill"
        }

        return Button(action: {
            if viewModel.activeTab != tab {
                Analytics.logTabSelected(tab.rawValue)
            }
            withAnimation(.easeInOut(duration: 0.2)) {
                viewModel.activeTab = tab
            }
        }) {
            HStack(spacing: 8) {
                Image(systemName: icon)
                    .font(.system(size: 13))
                Text(label)
                    .font(.system(size: 12, weight: .bold, design: .monospaced))
            }
            .foregroundColor(isActive ? .white : .white.opacity(0.4))
            .padding(.horizontal, 24)
            .padding(.vertical, 10)
            .frame(maxWidth: .infinity)
            .background(isActive ? accent.opacity(0.25) : Color.clear)
            .contentShape(Rectangle())
        }
        .buttonStyle(PlainButtonStyle())
    }

    // MARK: - Active Tab Content

    @ViewBuilder
    private func _buildActiveTab() -> some View {
        switch viewModel.activeTab {
        case .steam: _buildSteamTab()
        case .local: _buildLocalTab()
        }
    }

    // MARK: - Steam Tab

    private func _buildSteamTab() -> some View {
        VStack(spacing: 14) {
            _buildSteamCredentials()
            _buildSteamConsole()
            _buildSteamStatus()
        }
        .padding(20)
        .background(Color.black.opacity(0.4))
        .clipShape(RoundedRectangle(cornerRadius: 8))
        .overlay(RoundedRectangle(cornerRadius: 8).stroke(accent.opacity(0.2), lineWidth: 1))
    }

    private func _buildSteamCredentials() -> some View {
        VStack(alignment: .leading, spacing: 10) {
            Text(L10n.steam.credentials)
                .font(.system(size: 12, weight: .bold, design: .monospaced))
                .foregroundColor(.white.opacity(0.6))

            HStack(spacing: 12) {
                _buildTextField(placeholder: L10n.steam.usernamePlaceholder, text: $viewModel.steamUsername, isSecure: false)
                _buildPasswordField()

                if case .waitingSteamGuard = viewModel.steamCMD.state {
                    _buildTextField(placeholder: "Guard Code", text: $viewModel.steamCMD.steamGuardCode, isSecure: false)
                        .frame(width: 110)

                    Button(action: { viewModel.steamCMD.submitSteamGuard() }) {
                        Text(L10n.steam.submit)
                            .font(.system(size: 11, weight: .bold, design: .monospaced))
                            .foregroundColor(.white)
                            .padding(.horizontal, 12)
                            .padding(.vertical, 8)
                            .background(neonGreen.opacity(0.3))
                            .overlay(RoundedRectangle(cornerRadius: 4).stroke(neonGreen, lineWidth: 1))
                            .clipShape(RoundedRectangle(cornerRadius: 4))
                    }
                    .buttonStyle(PlainButtonStyle())
                }
            }

            HStack(spacing: 12) {
                _buildSteamActionButton()

                if viewModel.steamCMD.state.isRunning {
                    Button(action: { viewModel.steamCMD.cancel() }) {
                        Text(L10n.steam.cancel)
                            .font(.system(size: 11, weight: .bold, design: .monospaced))
                            .foregroundColor(.red)
                            .padding(.horizontal, 16)
                            .padding(.vertical, 8)
                            .background(Color.red.opacity(0.1))
                            .overlay(RoundedRectangle(cornerRadius: 4).stroke(Color.red.opacity(0.5), lineWidth: 1))
                            .clipShape(RoundedRectangle(cornerRadius: 4))
                    }
                    .buttonStyle(PlainButtonStyle())
                }
            }
        }
    }

    @ViewBuilder
    private func _buildSteamActionButton() -> some View {
        let canStart = !viewModel.steamUsername.isEmpty
            && !viewModel.steamPassword.isEmpty
            && !viewModel.steamCMD.state.isRunning
            && !viewModel.modInstaller.isBusy

        Button(action: {
            viewModel.saveCredentials()
            viewModel.steamCMD.startDownload(
                username: viewModel.steamUsername,
                password: viewModel.steamPassword
            )
        }) {
            HStack(spacing: 6) {
                Image(systemName: "arrow.down.circle.fill")
                    .font(.system(size: 13))
                Text(L10n.steam.download)
                    .font(.system(size: 12, weight: .bold, design: .monospaced))
            }
            .foregroundColor(canStart ? .white : .white.opacity(0.3))
            .padding(.horizontal, 20)
            .padding(.vertical, 8)
            .background(canStart ? accent.opacity(0.25) : Color.white.opacity(0.05))
            .overlay(RoundedRectangle(cornerRadius: 4).stroke(canStart ? accent : Color.white.opacity(0.1), lineWidth: 1))
            .clipShape(RoundedRectangle(cornerRadius: 4))
        }
        .buttonStyle(PlainButtonStyle())
        .disabled(!canStart)
    }

    private func _buildPasswordField() -> some View {
        _buildTextField(placeholder: L10n.steam.passwordPlaceholder, text: $viewModel.steamPassword, isSecure: true)
            .overlay(_buildStoredPasswordOverlay())
    }

    @ViewBuilder
    private func _buildStoredPasswordOverlay() -> some View {
        if viewModel.hasStoredPassword {
            Button(action: { viewModel.isAskingAboutStoredPassword = true }) {
                Color.clear.contentShape(Rectangle())
            }
            .buttonStyle(PlainButtonStyle())
            .help(L10n.steam.storedPasswordHint)
        }
    }

    private func _buildTextField(placeholder: String, text: Binding<String>, isSecure: Bool) -> some View {
        Group {
            if isSecure {
                SecureField(placeholder, text: text)
            } else {
                TextField(placeholder, text: text)
            }
        }
        .textFieldStyle(PlainTextFieldStyle())
        .font(.system(size: 13, design: .monospaced))
        .foregroundColor(.white)
        .padding(.horizontal, 10)
        .padding(.vertical, 7)
        .background(Color.black.opacity(0.5))
        .overlay(RoundedRectangle(cornerRadius: 4).stroke(accent.opacity(0.3), lineWidth: 1))
        .clipShape(RoundedRectangle(cornerRadius: 4))
    }

    private func _buildSteamConsole() -> some View {
        ScrollViewReader { proxy in
            ScrollView {
                Text(viewModel.steamCMD.consoleLog.isEmpty ? "Awaiting command..." : viewModel.steamCMD.consoleLog)
                    .font(.system(size: 11, design: .monospaced))
                    .foregroundColor(neonGreen.opacity(0.8))
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .padding(10)
                    .id("consoleBottom")
            }
            .frame(height: 120)
            .background(Color.black.opacity(0.7))
            .clipShape(RoundedRectangle(cornerRadius: 4))
            .overlay(RoundedRectangle(cornerRadius: 4).stroke(neonGreen.opacity(0.15), lineWidth: 1))
            .onChange(of: viewModel.steamCMD.consoleLog) { _ in
                withAnimation {
                    proxy.scrollTo("consoleBottom", anchor: .bottom)
                }
            }
        }
    }

    private func _buildSteamStatus() -> some View {
        HStack(spacing: 8) {
            Circle()
                .fill(_statusColor())
                .frame(width: 8, height: 8)
                .shadow(color: _statusColor(), radius: 4)

            Text(viewModel.steamCMD.state.statusText)
                .font(.system(size: 11, weight: .medium, design: .monospaced))
                .foregroundColor(.white.opacity(0.7))
                .lineLimit(1)

            Spacer()
        }
    }

    private func _statusColor() -> Color {
        switch viewModel.steamCMD.state {
        case .idle, .waitingForCredentials: return .gray
        case .installingRosetta, .downloadingSteamCMD, .authenticating, .downloading, .validating, .waitingSteamGuard, .downloadingPatch, .unpackingPatch: return .orange
        case .completed: return neonGreen
        case .failed: return .red
        }
    }

    // MARK: - Local Tab

    private func _buildLocalTab() -> some View {
        VStack(alignment: .leading, spacing: 14) {
            Text(L10n.local.path)
                .font(.system(size: 13, weight: .bold, design: .monospaced))
                .foregroundColor(.white.opacity(0.7))

            HStack(spacing: 12) {
                Text(viewModel.installPath.isEmpty ? "NO SIGNAL — AWAITING FOLDER TARGET" : viewModel.installPath)
                    .font(.system(size: 13, weight: .medium, design: .monospaced))
                    .foregroundColor(.white)
                    .lineLimit(1)
                    .truncationMode(.middle)
                    .padding(.horizontal, 14)
                    .padding(.vertical, 10)
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .background(darkPanel)
                    .overlay(RoundedRectangle(cornerRadius: 4).stroke(accent.opacity(0.5), lineWidth: 1.5))
                    .clipShape(RoundedRectangle(cornerRadius: 4))

                Button(action: { viewModel.chooseFolder() }) {
                    Text(L10n.local.locate)
                        .font(.system(size: 13, weight: .bold, design: .monospaced))
                        .foregroundColor(.white)
                        .padding(.horizontal, 18)
                        .padding(.vertical, 10)
                        .background(darkPanel)
                        .overlay(RoundedRectangle(cornerRadius: 4).stroke(accent, lineWidth: 1.5))
                        .clipShape(RoundedRectangle(cornerRadius: 4))
                }
                .buttonStyle(PlainButtonStyle())
                .disabled(viewModel.assetPatcher.state.isRunning || viewModel.modInstaller.isBusy)
            }

            if !viewModel.installPath.isEmpty && !viewModel.isPathValid {
                _buildLocalValidationWarning()
            }

            if viewModel.assetPatcher.state.isRunning || viewModel.assetPatcher.state == .completed {
                _buildPatchConsole()
                _buildPatchStatus()
            }
        }
        .padding(20)
        .background(Color.black.opacity(0.4))
        .clipShape(RoundedRectangle(cornerRadius: 8))
        .overlay(RoundedRectangle(cornerRadius: 8).stroke(accent.opacity(0.2), lineWidth: 1))
    }

    private func _buildLocalValidationWarning() -> some View {
        HStack(spacing: 8) {
            Image(systemName: "exclamationmark.triangle.fill")
                .foregroundColor(.red.opacity(0.8))
                .font(.system(size: 14))

            Text(L10n.local.invalidTarget)
                .font(.system(size: 11, weight: .medium, design: .monospaced))
                .foregroundColor(.red.opacity(0.8))
        }
        .padding(10)
        .background(Color.red.opacity(0.1))
        .clipShape(RoundedRectangle(cornerRadius: 4))
        .overlay(RoundedRectangle(cornerRadius: 4).stroke(Color.red.opacity(0.3), lineWidth: 1))
    }

    // MARK: - Bottom Action

    @ViewBuilder
    private func _buildBottomAction() -> some View {
        if let installing = viewModel.installingMod {
            _buildModBusyAction(installing)
        } else if viewModel.selectedProfile.isMod {
            _buildModAction(viewModel.selectedProfile)
        } else if viewModel.canLaunch {
            _buildLaunchButton()
        } else if viewModel.assetPatcher.state.isRunning {
            _buildPatchingProgressButton()
        } else if viewModel.needsPatching {
            _buildPatchButton()
        } else {
            _buildTargetRequiredHint()
        }
    }

    // MARK: - Mod Actions

    private func _buildModBusyAction(_ profile: GameProfile) -> some View {
        let state = viewModel.installingModState
        let isRemoving = state == .removing

        return VStack(spacing: 8) {
            Text(String(
                format: isRemoving ? L10n.mod.removingTitle : L10n.mod.installingTitle,
                profile.shortName
            ))
                .font(.system(size: 13, weight: .bold, design: .monospaced))
                .foregroundColor(profile.theme.accentSoft)

            _buildModProgress(profile, state: state)

            if !isRemoving {
                Text(L10n.mod.installingHint)
                    .font(.system(size: 10, weight: .medium, design: .monospaced))
                    .foregroundColor(.white.opacity(0.55))
                    .multilineTextAlignment(.center)
                    .frame(maxWidth: 420)
            }
        }
    }

    @ViewBuilder
    private func _buildModAction(_ profile: GameProfile) -> some View {
        let state = viewModel.modState(profile)

        VStack(spacing: 10) {
            switch state {
            case .downloading, .unpacking, .removing:
                _buildModProgress(profile, state: state)
            case .completed:
                _buildModInstalled(profile)
            case .failed(let message):
                _buildModInstallButton(profile, errorText: message)
            case .idle:
                _buildModInstallButton(profile, errorText: nil)
            }
        }
    }

    private func _buildModProgress(_ profile: GameProfile, state: ModInstallState) -> some View {
        VStack(spacing: 8) {
            ProgressView(value: state.fraction)
                .progressViewStyle(LinearProgressViewStyle(tint: profile.theme.accent))
                .frame(width: 320)

            Text(state.statusText)
                .font(.system(size: 12, weight: .bold, design: .monospaced))
                .foregroundColor(profile.theme.accentSoft)
        }
        .padding(.horizontal, 30)
        .padding(.vertical, 14)
        .background(profile.theme.accent.opacity(0.08))
        .clipShape(RoundedRectangle(cornerRadius: 8))
        .overlay(RoundedRectangle(cornerRadius: 8).stroke(profile.theme.accent.opacity(0.4), lineWidth: 1))
    }

    private func _buildModInstallButton(_ profile: GameProfile, errorText: String?) -> some View {
        let sizeText = _modSizeText(profile)
        let isDamaged = viewModel.isModDamaged(profile)
        let damageText = isDamaged
            ? String(format: L10n.mod.damaged, viewModel.missingModFileCount(profile))
            : nil

        return VStack(spacing: 8) {
            if let text = errorText ?? damageText {
                Text(text)
                    .font(.system(size: 11, weight: .medium, design: .monospaced))
                    .foregroundColor(isDamaged && errorText == nil ? .orange.opacity(0.85) : .red.opacity(0.85))
                    .lineLimit(2)
            }

            Button(action: { viewModel.installMod(profile) }) {
                HStack(spacing: 10) {
                    Image(systemName: isDamaged ? "wrench.and.screwdriver.fill" : "arrow.down.circle.fill")
                    Text("\(isDamaged ? L10n.mod.repair : L10n.mod.install) \(profile.shortName)")
                }
                .font(.system(size: 20, weight: .bold, design: .monospaced))
                .foregroundColor(.white)
                .padding(.horizontal, 40)
                .padding(.vertical, 12)
                .background(profile.theme.accent.opacity(0.25))
                .overlay(RoundedRectangle(cornerRadius: 8).stroke(profile.theme.accent, lineWidth: 2))
                .clipShape(RoundedRectangle(cornerRadius: 8))
            }
            .buttonStyle(PlainButtonStyle())
            .disabled(viewModel.modInstaller.isBusy)
            .opacity(viewModel.modInstaller.isBusy ? 0.4 : 1.0)

            Text(sizeText)
                .font(.system(size: 10, weight: .medium, design: .monospaced))
                .foregroundColor(.white.opacity(0.5))
        }
    }

    private func _buildModInstalled(_ profile: GameProfile) -> some View {
        VStack(spacing: 8) {
            HStack(spacing: 8) {
                Image(systemName: "checkmark.seal.fill")
                    .foregroundColor(neonGreen)
                Text(String(format: L10n.mod.installed, profile.displayName))
                    .font(.system(size: 12, weight: .bold, design: .monospaced))
                    .foregroundColor(.white.opacity(0.75))
            }

            _buildLaunchButton()

            HStack(spacing: 12) {
                _buildModSecondaryButton(title: L10n.mod.reinstall, color: profile.theme.accentSoft) {
                    viewModel.requestModReinstall(profile)
                }

                _buildModSecondaryButton(title: L10n.mod.remove, color: .red.opacity(0.8)) {
                    viewModel.requestModRemoval(profile)
                }
            }
        }
        .padding(.horizontal, 30)
        .padding(.vertical, 12)
        .background(Color.black.opacity(0.35))
        .clipShape(RoundedRectangle(cornerRadius: 8))
        .overlay(RoundedRectangle(cornerRadius: 8).stroke(profile.theme.accent.opacity(0.35), lineWidth: 1))
    }

    private func _buildModSecondaryButton(
        title: String,
        color: Color,
        action: @escaping () -> Void
    ) -> some View {
        Button(action: action) {
            Text(title)
                .font(.system(size: 11, weight: .bold, design: .monospaced))
                .foregroundColor(color)
                .padding(.horizontal, 14)
                .padding(.vertical, 6)
                .overlay(RoundedRectangle(cornerRadius: 4).stroke(color.opacity(0.5), lineWidth: 1))
        }
        .buttonStyle(PlainButtonStyle())
        .disabled(viewModel.modInstaller.isBusy)
    }

    private func _modDownloadSizeText(_ profile: GameProfile) -> String {
        guard let mod = profile.mod else { return "" }

        return String(format: "%.1f GB", Double(mod.downloadSizeMB) / 1024)
    }

    private func _modSizeText(_ profile: GameProfile) -> String {
        guard let mod = profile.mod else { return "" }

        let download = String(format: "%.1f GB", Double(mod.downloadSizeMB) / 1024)
        let disk = String(format: "%.1f GB", Double(mod.diskSizeMB) / 1024)
        let parts = mod.partCount > 1 ? String(format: L10n.mod.sizeParts, mod.partCount) : ""

        return String(format: L10n.mod.sizeInfo, download, disk) + parts
    }

    private func _buildLaunchButton() -> some View {
        Button(action: { viewModel.launchGame() }) {
            HStack(spacing: 10) {
                if viewModel.isLaunching {
                    ProgressView()
                        .scaleEffect(0.7)
                        .progressViewStyle(CircularProgressViewStyle(tint: .white))
                    Text(L10n.action.initialize)
                } else {
                    Image(systemName: "play.fill")
                    Text(L10n.action.launch)
                }
            }
            .font(.system(size: 24, weight: .bold, design: .monospaced))
            .foregroundColor(.white)
            .padding(.horizontal, 50)
            .padding(.vertical, 14)
            .background(
                LinearGradient(
                    colors: [accent.opacity(0.3), accent.opacity(0.15)],
                    startPoint: .leading,
                    endPoint: .trailing
                )
            )
            .overlay(RoundedRectangle(cornerRadius: 8).stroke(accent, lineWidth: 2))
            .clipShape(RoundedRectangle(cornerRadius: 8))
            .shadow(color: accent.opacity(0.5), radius: 12)
        }
        .buttonStyle(PlainButtonStyle())
        .disabled(viewModel.isLaunching || !viewModel.canLaunch)
        .opacity(viewModel.canLaunch ? 1.0 : 0.45)
    }

    private func _buildTargetRequiredHint() -> some View {
        HStack(spacing: 8) {
            if viewModel.activeTab == .local {
                if let imgPath = Bundle.main.path(forResource: "dir_image", ofType: "png"),
                   let nsImg = NSImage(contentsOfFile: imgPath) {
                    Image(nsImage: nsImg)
                        .resizable()
                        .scaledToFit()
                        .frame(maxHeight: 60)
                        .clipShape(RoundedRectangle(cornerRadius: 4))
                        .overlay(RoundedRectangle(cornerRadius: 4).stroke(Color.orange.opacity(0.4), lineWidth: 1))
                }
            }

            Text(viewModel.activeTab == .steam
                 ? "DOWNLOAD GAME ASSETS VIA STEAM TO ENABLE LAUNCH"
                 : L10n.local.selectHint)
                .font(.system(size: 12, weight: .bold, design: .monospaced))
                .foregroundColor(.orange.opacity(0.8))
                .multilineTextAlignment(.center)
        }
        .padding(14)
        .background(Color.orange.opacity(0.08))
        .clipShape(RoundedRectangle(cornerRadius: 6))
        .overlay(RoundedRectangle(cornerRadius: 6).stroke(Color.orange.opacity(0.3), lineWidth: 1))
    }

    // MARK: - Patch Actions (Local Archive)

    private func _buildPatchButton() -> some View {
        Button(action: { viewModel.requestPatching() }) {
            HStack(spacing: 10) {
                Image(systemName: "arrow.down.doc.fill")
                Text(L10n.action.patch)
            }
            .font(.system(size: 24, weight: .bold, design: .monospaced))
            .foregroundColor(.white)
            .padding(.horizontal, 50)
            .padding(.vertical, 14)
            .background(
                LinearGradient(
                    colors: [Color.orange.opacity(0.3), Color.orange.opacity(0.15)],
                    startPoint: .leading,
                    endPoint: .trailing
                )
            )
            .overlay(RoundedRectangle(cornerRadius: 8).stroke(Color.orange, lineWidth: 2))
            .clipShape(RoundedRectangle(cornerRadius: 8))
            .shadow(color: Color.orange.opacity(0.5), radius: 12)
        }
        .buttonStyle(PlainButtonStyle())
    }

    private func _buildPatchingProgressButton() -> some View {
        HStack(spacing: 10) {
            ProgressView()
                .scaleEffect(0.7)
                .progressViewStyle(CircularProgressViewStyle(tint: .orange))
            Text(viewModel.assetPatcher.state.statusText)
                .font(.system(size: 14, weight: .bold, design: .monospaced))
                .foregroundColor(.orange.opacity(0.8))
        }
        .padding(.horizontal, 30)
        .padding(.vertical, 14)
        .background(Color.orange.opacity(0.08))
        .clipShape(RoundedRectangle(cornerRadius: 8))
        .overlay(RoundedRectangle(cornerRadius: 8).stroke(Color.orange.opacity(0.3), lineWidth: 1))
    }

    private func _buildPatchConsole() -> some View {
        ScrollViewReader { proxy in
            ScrollView {
                Text(viewModel.assetPatcher.consoleLog.isEmpty ? "Awaiting patch process..." : viewModel.assetPatcher.consoleLog)
                    .font(.system(size: 11, design: .monospaced))
                    .foregroundColor(neonGreen.opacity(0.8))
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .padding(10)
                    .id("patchConsoleBottom")
            }
            .frame(height: 100)
            .background(Color.black.opacity(0.7))
            .clipShape(RoundedRectangle(cornerRadius: 4))
            .overlay(RoundedRectangle(cornerRadius: 4).stroke(neonGreen.opacity(0.15), lineWidth: 1))
            .onChange(of: viewModel.assetPatcher.consoleLog) { _ in
                withAnimation {
                    proxy.scrollTo("patchConsoleBottom", anchor: .bottom)
                }
            }
        }
    }

    private func _buildPatchStatus() -> some View {
        HStack(spacing: 8) {
            Circle()
                .fill(_patchStatusColor())
                .frame(width: 8, height: 8)
                .shadow(color: _patchStatusColor(), radius: 4)

            Text(viewModel.assetPatcher.state.statusText)
                .font(.system(size: 11, weight: .medium, design: .monospaced))
                .foregroundColor(.white.opacity(0.7))
                .lineLimit(1)

            Spacer()
        }
    }

    private func _patchStatusColor() -> Color {
        switch viewModel.assetPatcher.state {
        case .idle: return .gray
        case .cleaning, .downloadingPatch, .unpacking: return .orange
        case .completed: return neonGreen
        case .failed: return .red
        }
    }

    // MARK: - Update Banner

    private func _buildUpdateBanner(_ update: AppUpdate) -> some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Image(systemName: "arrow.down.circle.fill")
                    .foregroundColor(neonGreen)
                    .font(.system(size: 16))

                Text(L10n.update.available.replacingOccurrences(of: "%@", with: update.version))
                    .font(.system(size: 13, weight: .bold, design: .monospaced))
                    .foregroundColor(.white)

                Spacer()

                Button(action: {
                    Analytics.logUpdateDismissed(version: update.version)
                    viewModel.isUpdateDismissed = true
                }) {
                    Image(systemName: "xmark")
                        .font(.system(size: 11, weight: .bold))
                        .foregroundColor(.white.opacity(0.5))
                }
                .buttonStyle(PlainButtonStyle())
            }

            Text(update.releaseNotes)
                .font(.system(size: 11, design: .monospaced))
                .foregroundColor(.white.opacity(0.7))
                .lineLimit(3)

            HStack(spacing: 10) {
                Button(action: {
                    Analytics.logUpdateOpened(version: update.version)

                    if let url = URL(string: update.downloadURL) {
                        NSWorkspace.shared.open(url)
                    }
                }) {
                    HStack(spacing: 6) {
                        Image(systemName: "arrow.down.to.line")
                            .font(.system(size: 11))
                        Text(L10n.update.download)
                            .font(.system(size: 11, weight: .bold, design: .monospaced))
                    }
                    .foregroundColor(.white)
                    .padding(.horizontal, 14)
                    .padding(.vertical, 6)
                    .background(neonGreen.opacity(0.2))
                    .overlay(RoundedRectangle(cornerRadius: 4).stroke(neonGreen.opacity(0.6), lineWidth: 1))
                    .clipShape(RoundedRectangle(cornerRadius: 4))
                }
                .buttonStyle(PlainButtonStyle())

                Button(action: {
                    Analytics.logLinkOpened(target: "website", location: "update_banner")

                    if let url = URL(string: "https://general-online-zh.web.app") {
                        NSWorkspace.shared.open(url)
                    }
                }) {
                    HStack(spacing: 6) {
                        Image(systemName: "safari")
                            .font(.system(size: 11))
                        Text(L10n.update.details)
                            .font(.system(size: 11, weight: .bold, design: .monospaced))
                    }
                    .foregroundColor(.white.opacity(0.7))
                    .padding(.horizontal, 14)
                    .padding(.vertical, 6)
                    .overlay(RoundedRectangle(cornerRadius: 4).stroke(.white.opacity(0.2), lineWidth: 1))
                    .clipShape(RoundedRectangle(cornerRadius: 4))
                }
                .buttonStyle(PlainButtonStyle())
            }
        }
        .padding(14)
        .background(neonGreen.opacity(0.08))
        .clipShape(RoundedRectangle(cornerRadius: 8))
        .overlay(RoundedRectangle(cornerRadius: 8).stroke(neonGreen.opacity(0.3), lineWidth: 1))
    }

    // MARK: - Footer

    private func _buildFooter() -> some View {
        HStack(spacing: 12) {
            if let imgPath = Bundle.main.path(forResource: "author_logo", ofType: "png"),
               let nsImg = NSImage(contentsOfFile: imgPath) {
                Image(nsImage: nsImg)
                    .resizable()
                    .scaledToFit()
                    .frame(width: 36, height: 36)
                    .clipShape(Circle())
                    .overlay(Circle().stroke(Color.white.opacity(0.2), lineWidth: 1))
                    .shadow(color: .black, radius: 2)
            }

            VStack(alignment: .leading, spacing: 3) {
                Text(L10n.footer.author)
                    .font(.system(size: 12, weight: .bold, design: .monospaced))
                    .foregroundColor(.white.opacity(0.75))
                    .shadow(color: .black, radius: 1, x: 1, y: 1)

                HStack(spacing: 8) {
                    _buildFooterLink(title: "Website", url: "https://general-online-zh.web.app")
                    Text("|").foregroundColor(.white.opacity(0.3)).font(.system(size: 10))
                    _buildFooterLink(title: "Telegram", url: "https://t.me/GeneralsOnlineMacOSChannel")
                }
            }
            Spacer()
        }
        .padding(.horizontal, 8)
    }

    private func _buildFooterLink(title: String, url: String) -> some View {
        Button(action: {
            Analytics.logLinkOpened(target: title.lowercased(), location: "footer")
            if let link = URL(string: url) { NSWorkspace.shared.open(link) }
        }) {
            Text(title)
                .font(.system(size: 10, weight: .medium, design: .monospaced))
                .foregroundColor(accent)
                .underline()
                .shadow(color: .black, radius: 1)
        }
        .buttonStyle(PlainButtonStyle())
        .onHover { inside in
            if inside { NSCursor.pointingHand.push() } else { NSCursor.pop() }
        }
    }
}

// MARK: - Alert Helper

enum LauncherAlert: Identifiable {
    case message(String)
    case purchase(username: String)
    case rosetta
    case patchConfirmation
    case modConfirmation(LauncherViewModel.ModConfirmation)
    case storedPassword(account: String)

    var id: String {
        switch self {
        case .message(let text): return "message:\(text)"
        case .purchase: return "purchase"
        case .rosetta: return "rosetta"
        case .patchConfirmation: return "patch"
        case .modConfirmation(let confirmation): return "mod:\(confirmation.id)"
        case .storedPassword: return "storedPassword"
        }
    }
}
