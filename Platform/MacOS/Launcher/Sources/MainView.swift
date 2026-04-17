import SwiftUI
import AppKit

// MARK: - View Model

class LauncherViewModel: ObservableObject {
    enum Tab: String, CaseIterable {
        case steam = "Steam"
        case local = "Local Archive"
    }

    @Published var activeTab: Tab = .steam
    @Published var installPath: String = UserDefaults.standard.string(forKey: "GENERALS_INSTALL_PATH") ?? ""
    @Published var isLaunching: Bool = false
    @Published var alertMessage: String? = nil

    @Published var steamUsername: String = ""
    @Published var steamPassword: String = ""

    var steamCMD = SteamCMDManager()

    var isPathValid: Bool {
        guard !installPath.isEmpty else { return false }
        return _validateGameFolder(at: URL(fileURLWithPath: installPath))
    }

    var canLaunch: Bool {
        switch activeTab {
        case .steam: return steamCMD.areAssetsValid
        case .local: return isPathValid
        }
    }

    var effectiveInstallPath: String {
        switch activeTab {
        case .steam: return steamCMD.assetsDir.path
        case .local: return installPath
        }
    }

    func chooseFolder() {
        let panel = NSOpenPanel()
        panel.canChooseFiles = false
        panel.canChooseDirectories = true
        panel.allowsMultipleSelection = false
        panel.message = NSLocalizedString("Select the Windows Game Folder (containing .big files)", comment: "")

        if panel.runModal() == .OK, let url = panel.url {
            DispatchQueue.main.async {
                self.installPath = url.path
                UserDefaults.standard.set(self.installPath, forKey: "GENERALS_INSTALL_PATH")
            }
        }
    }

    func launchGame() {
        guard canLaunch else { return }
        isLaunching = true

        guard let executableURL = Bundle.main.executableURL?.deletingLastPathComponent().appendingPathComponent("GeneralsOnlineZH") else {
            isLaunching = false
            return
        }

        guard FileManager.default.fileExists(atPath: executableURL.path) else {
            alertMessage = "Engine binary not found at \(executableURL.path)"
            isLaunching = false
            return
        }

        let task = Process()
        task.executableURL = executableURL
        task.currentDirectoryURL = executableURL.deletingLastPathComponent()

        var env = ProcessInfo.processInfo.environment
        env["GENERALS_INSTALL_PATH"] = effectiveInstallPath
        task.environment = env

        do {
            try task.run()

            DispatchQueue.global().async {
                Thread.sleep(forTimeInterval: 0.5)
                DispatchQueue.main.async {
                    if let app = NSRunningApplication(processIdentifier: task.processIdentifier) {
                        app.activate(options: .activateIgnoringOtherApps)
                    }
                    NSApplication.shared.terminate(nil)
                }
            }
        } catch {
            alertMessage = "Failed to launch game: \(error.localizedDescription)"
            isLaunching = false
        }
    }

    private func _validateGameFolder(at url: URL) -> Bool {
        let fm = FileManager.default
        guard let items = try? fm.contentsOfDirectory(at: url, includingPropertiesForKeys: [.isDirectoryKey], options: .skipsHiddenFiles) else {
            return false
        }

        var hasZH = false
        var hasBase = false

        for itemURL in items {
            guard let isDir = try? itemURL.resourceValues(forKeys: [.isDirectoryKey]).isDirectory, isDir else { continue }
            guard let subItems = try? fm.contentsOfDirectory(atPath: itemURL.path) else { continue }

            if subItems.contains(where: { $0.lowercased() == "inizh.big" }) { hasZH = true }
            else if subItems.contains(where: { $0.lowercased() == "ini.big" }) { hasBase = true }

            if hasZH && hasBase { return true }
        }

        return false
    }
}

// MARK: - Window Accessor

struct WindowAccessor: NSViewRepresentable {
    func makeNSView(context: Context) -> NSView {
        let view = NSView()
        DispatchQueue.main.async {
            guard let window = view.window else { return }
            window.titlebarAppearsTransparent = true
            window.titleVisibility = .hidden
            window.styleMask.insert(.fullSizeContentView)
        }
        return view
    }
    func updateNSView(_ nsView: NSView, context: Context) {}
}

// MARK: - Main View

struct MainView: View {
    @StateObject private var viewModel = LauncherViewModel()

    private let neonBlue = Color(red: 0.1, green: 0.5, blue: 1.0)
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

                VStack(spacing: 0) {
                    _buildHeader()
                        .padding(.top, 30)

                    _buildTabBar()
                        .padding(.top, 20)

                    _buildActiveTab()
                        .padding(.horizontal, 40)
                        .padding(.top, 16)

                    Spacer()

                    _buildBottomAction()
                        .padding(.bottom, 12)

                    _buildFooter()
                        .padding(.bottom, 20)
                }
                .padding(.horizontal, 20)
            }
            .frame(width: geometry.size.width, height: geometry.size.height)
            .edgesIgnoringSafeArea(.all)
        }
        .alert(item: Binding<AlertItem?>(
            get: { viewModel.alertMessage.map { AlertItem(message: $0) } },
            set: { _ in viewModel.alertMessage = nil }
        )) { alert in
            Alert(title: Text("Launch Error"), message: Text(alert.message), dismissButton: .default(Text("OK")))
        }
    }

    // MARK: - Background

    @ViewBuilder
    private func _buildBackground(size: CGSize) -> some View {
        if let bgPath = Bundle.main.path(forResource: "background", ofType: "png"),
           let nsImage = NSImage(contentsOfFile: bgPath) {
            Image(nsImage: nsImage)
                .resizable()
                .scaledToFill()
                .frame(width: size.width, height: size.height)
                .clipped()
                .overlay(Color.black.opacity(0.55))
        }
    }

    // MARK: - Header

    private func _buildHeader() -> some View {
        VStack(spacing: 4) {
            Text("GENERALS ONLINE")
                .font(.system(size: 48, weight: .black))
                .foregroundColor(.white)
                .shadow(color: neonBlue, radius: 15)

            Text("COMMUNITY MAC PORT")
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
        .overlay(RoundedRectangle(cornerRadius: 6).stroke(neonBlue.opacity(0.3), lineWidth: 1))
        .padding(.horizontal, 40)
    }

    private func _buildTabButton(_ tab: LauncherViewModel.Tab) -> some View {
        let isActive = viewModel.activeTab == tab
        let label: String
        let icon: String

        switch tab {
        case .steam:
            label = "STEAM (RECOMMENDED)"
            icon = "arrow.down.circle.fill"
        case .local:
            label = "LOCAL ARCHIVE"
            icon = "folder.fill"
        }

        return Button(action: {
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
            .background(isActive ? neonBlue.opacity(0.25) : Color.clear)
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
        .overlay(RoundedRectangle(cornerRadius: 8).stroke(neonBlue.opacity(0.2), lineWidth: 1))
    }

    private func _buildSteamCredentials() -> some View {
        VStack(alignment: .leading, spacing: 10) {
            Text("STEAM CREDENTIALS")
                .font(.system(size: 12, weight: .bold, design: .monospaced))
                .foregroundColor(.white.opacity(0.6))

            HStack(spacing: 12) {
                _buildTextField(placeholder: "Username", text: $viewModel.steamUsername, isSecure: false)
                _buildTextField(placeholder: "Password", text: $viewModel.steamPassword, isSecure: true)

                if case .waitingSteamGuard = viewModel.steamCMD.state {
                    _buildTextField(placeholder: "Guard Code", text: $viewModel.steamCMD.steamGuardCode, isSecure: false)
                        .frame(width: 110)

                    Button(action: { viewModel.steamCMD.submitSteamGuard() }) {
                        Text("SUBMIT")
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
                        Text("CANCEL")
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

        Button(action: {
            viewModel.steamCMD.startDownload(
                username: viewModel.steamUsername,
                password: viewModel.steamPassword
            )
        }) {
            HStack(spacing: 6) {
                Image(systemName: "arrow.down.circle.fill")
                    .font(.system(size: 13))
                Text("DOWNLOAD ASSETS")
                    .font(.system(size: 12, weight: .bold, design: .monospaced))
            }
            .foregroundColor(canStart ? .white : .white.opacity(0.3))
            .padding(.horizontal, 20)
            .padding(.vertical, 8)
            .background(canStart ? neonBlue.opacity(0.25) : Color.white.opacity(0.05))
            .overlay(RoundedRectangle(cornerRadius: 4).stroke(canStart ? neonBlue : Color.white.opacity(0.1), lineWidth: 1))
            .clipShape(RoundedRectangle(cornerRadius: 4))
        }
        .buttonStyle(PlainButtonStyle())
        .disabled(!canStart)
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
        .overlay(RoundedRectangle(cornerRadius: 4).stroke(neonBlue.opacity(0.3), lineWidth: 1))
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
        case .downloadingSteamCMD, .authenticating, .downloading, .validating, .waitingSteamGuard: return .orange
        case .completed: return neonGreen
        case .failed: return .red
        }
    }

    // MARK: - Local Tab

    private func _buildLocalTab() -> some View {
        VStack(alignment: .leading, spacing: 14) {
            Text("TACTICAL DATA PATH:")
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
                    .overlay(RoundedRectangle(cornerRadius: 4).stroke(neonBlue.opacity(0.5), lineWidth: 1.5))
                    .clipShape(RoundedRectangle(cornerRadius: 4))

                Button(action: { viewModel.chooseFolder() }) {
                    Text("LOCATE")
                        .font(.system(size: 13, weight: .bold, design: .monospaced))
                        .foregroundColor(.white)
                        .padding(.horizontal, 18)
                        .padding(.vertical, 10)
                        .background(darkPanel)
                        .overlay(RoundedRectangle(cornerRadius: 4).stroke(neonBlue, lineWidth: 1.5))
                        .clipShape(RoundedRectangle(cornerRadius: 4))
                }
                .buttonStyle(PlainButtonStyle())
            }

            if !viewModel.installPath.isEmpty && !viewModel.isPathValid {
                _buildLocalValidationWarning()
            }
        }
        .padding(20)
        .background(Color.black.opacity(0.4))
        .clipShape(RoundedRectangle(cornerRadius: 8))
        .overlay(RoundedRectangle(cornerRadius: 8).stroke(neonBlue.opacity(0.2), lineWidth: 1))
    }

    private func _buildLocalValidationWarning() -> some View {
        HStack(spacing: 8) {
            Image(systemName: "exclamationmark.triangle.fill")
                .foregroundColor(.red.opacity(0.8))
                .font(.system(size: 14))

            Text("INVALID TARGET — No ini.big / inizh.big detected in subdirectories")
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
        if viewModel.canLaunch {
            _buildLaunchButton()
        } else {
            _buildTargetRequiredHint()
        }
    }

    private func _buildLaunchButton() -> some View {
        Button(action: { viewModel.launchGame() }) {
            HStack(spacing: 10) {
                if viewModel.isLaunching {
                    ProgressView()
                        .scaleEffect(0.7)
                        .progressViewStyle(CircularProgressViewStyle(tint: .white))
                    Text("INITIALIZING...")
                } else {
                    Image(systemName: "play.fill")
                    Text("LAUNCH")
                }
            }
            .font(.system(size: 24, weight: .bold, design: .monospaced))
            .foregroundColor(.white)
            .padding(.horizontal, 50)
            .padding(.vertical, 14)
            .background(
                LinearGradient(
                    colors: [neonBlue.opacity(0.3), neonBlue.opacity(0.15)],
                    startPoint: .leading,
                    endPoint: .trailing
                )
            )
            .overlay(RoundedRectangle(cornerRadius: 8).stroke(neonBlue, lineWidth: 2))
            .clipShape(RoundedRectangle(cornerRadius: 8))
            .shadow(color: neonBlue.opacity(0.5), radius: 12)
        }
        .buttonStyle(PlainButtonStyle())
        .disabled(viewModel.isLaunching)
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
                 : "SELECT THE PARENT DIRECTORY CONTAINING BOTH GAME VERSIONS")
                .font(.system(size: 12, weight: .bold, design: .monospaced))
                .foregroundColor(.orange.opacity(0.8))
                .multilineTextAlignment(.center)
        }
        .padding(14)
        .background(Color.orange.opacity(0.08))
        .clipShape(RoundedRectangle(cornerRadius: 6))
        .overlay(RoundedRectangle(cornerRadius: 6).stroke(Color.orange.opacity(0.3), lineWidth: 1))
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
                Text("Ported by OKJI (Okladnoj)")
                    .font(.system(size: 12, weight: .bold, design: .monospaced))
                    .foregroundColor(.white.opacity(0.75))
                    .shadow(color: .black, radius: 1, x: 1, y: 1)

                HStack(spacing: 8) {
                    _buildFooterLink(title: "Website", url: "https://okladnoj-bio.web.app/")
                    Text("|").foregroundColor(.white.opacity(0.3)).font(.system(size: 10))
                    _buildFooterLink(title: "Telegram", url: "https://t.me/GeneralsOnlineMacOS")
                }
            }
            Spacer()
        }
        .padding(.horizontal, 20)
    }

    private func _buildFooterLink(title: String, url: String) -> some View {
        Button(action: {
            if let link = URL(string: url) { NSWorkspace.shared.open(link) }
        }) {
            Text(title)
                .font(.system(size: 10, weight: .medium, design: .monospaced))
                .foregroundColor(neonBlue)
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

struct AlertItem: Identifiable {
    let id = UUID()
    let message: String
}
