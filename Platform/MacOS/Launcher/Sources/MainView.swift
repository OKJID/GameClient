import SwiftUI
import AppKit

class LauncherViewModel: ObservableObject {
    @Published var installPath: String = UserDefaults.standard.string(forKey: "GENERALS_INSTALL_PATH") ?? ""
    @Published var isLaunching: Bool = false
    @Published var alertMessage: String? = nil
    
    var isPathValid: Bool {
        guard !installPath.isEmpty else { return false }
        let fm = FileManager.default
        let url = URL(fileURLWithPath: installPath)
        
        guard let items = try? fm.contentsOfDirectory(at: url, includingPropertiesForKeys: [.isDirectoryKey], options: .skipsHiddenFiles) else { return false }
        
        var hasZH = false
        var hasBase = false
        
        for itemURL in items {
            guard let isDir = try? itemURL.resourceValues(forKeys: [.isDirectoryKey]).isDirectory, isDir else { continue }
            guard let subItems = try? fm.contentsOfDirectory(atPath: itemURL.path) else { continue }
            
            let containsZH = subItems.contains { $0.lowercased() == "inizh.big" }
            let containsBase = subItems.contains { $0.lowercased() == "ini.big" }
            
            if containsZH {
                hasZH = true
            } else if containsBase {
                hasBase = true
            }
            
            if hasZH && hasBase { return true }
        }
        
        return false
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
        guard isPathValid else { return }
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
        env["GENERALS_INSTALL_PATH"] = installPath
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
}

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

struct MainView: View {
    @StateObject private var viewModel = LauncherViewModel()
    
    private let neonBlue = Color(red: 0.1, green: 0.5, blue: 1.0)
    private let darkPanel = Color.black.opacity(0.85)
    
    var body: some View {
        GeometryReader { geometry in
            ZStack {
                WindowAccessor().frame(width: 0, height: 0)
                
                _buildBackground(size: geometry.size)
                
                VStack(spacing: 35) {
                    Spacer()
                    _buildHeader()
                    Spacer()
                    _buildPathSelector()
                    Spacer()
                    _buildBottomAction()
                    Spacer()
                }
                .padding(30)
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
    
    @ViewBuilder
    private func _buildBackground(size: CGSize) -> some View {
        if let bgPath = Bundle.main.path(forResource: "background", ofType: "png"),
           let nsImage = NSImage(contentsOfFile: bgPath) {
            Image(nsImage: nsImage)
                .resizable()
                .scaledToFill()
                .frame(width: size.width, height: size.height)
                .clipped()
                .overlay(Color.black.opacity(0.4))
        }
    }
    
    private func _buildHeader() -> some View {
        VStack(spacing: 5) {
            Text("GENERALS ONLINE")
                .font(.system(size: 56, weight: .black, design: .default))
                .foregroundColor(.white)
                .shadow(color: neonBlue, radius: 15, x: 0, y: 0)
            
            Text("COMMUNITY MAC PORT (ALPHA)")
                .font(.system(size: 16, weight: .bold, design: .monospaced))
                .foregroundColor(.white)
                .shadow(color: .black, radius: 2, x: 0, y: 2)
        }
    }
    
    private func _buildPathSelector() -> some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("TACTICAL DATA PATH:")
                .font(.system(size: 14, weight: .bold, design: .monospaced))
                .foregroundColor(.white)
                .shadow(color: .black, radius: 2, x: 1, y: 1)
            
            HStack(spacing: 12) {
                Text(viewModel.installPath.isEmpty ? "NO SIGNAL - AWAITING FOLDER TARGET" : viewModel.installPath)
                    .font(.system(size: 14, weight: .medium, design: .monospaced))
                    .foregroundColor(.white)
                    .lineLimit(1)
                    .truncationMode(.middle)
                    .padding(.horizontal, 15)
                    .padding(.vertical, 12)
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .background(darkPanel)
                    .overlay(Rectangle().stroke(neonBlue, lineWidth: 2))
                
                Button(action: {
                    viewModel.chooseFolder()
                }) {
                    Text("LOCATE")
                        .font(.system(size: 14, weight: .bold, design: .monospaced))
                        .padding(.horizontal, 20)
                        .padding(.vertical, 12)
                        .background(darkPanel)
                        .overlay(Rectangle().stroke(neonBlue, lineWidth: 2))
                }
                .buttonStyle(PlainButtonStyle())
                .foregroundColor(.white)
            }
        }
        .padding(25)
        .background(Color.black.opacity(0.3))
        .padding(.horizontal, 40)
    }
    
    @ViewBuilder
    private func _buildBottomAction() -> some View {
        if viewModel.isPathValid {
            _buildLaunchButton()
        } else {
            _buildTargetRequiredAlert()
        }
    }
    
    private func _buildLaunchButton() -> some View {
        Button(action: {
            viewModel.launchGame()
        }) {
            Group {
                if viewModel.isLaunching {
                    Text("INITIALIZING...")
                } else {
                    Text("LAUNCH")
                }
            }
            .font(.system(size: 28, weight: .bold, design: .monospaced))
            .padding(.horizontal, 50)
            .padding(.vertical, 15)
            .background(darkPanel)
            .foregroundColor(.white)
            .overlay(Rectangle().stroke(neonBlue, lineWidth: 2))
            .shadow(color: neonBlue.opacity(0.6), radius: 10)
        }
        .buttonStyle(PlainButtonStyle())
        .disabled(viewModel.isLaunching)
    }
    
    private func _buildTargetRequiredAlert() -> some View {
        VStack(spacing: 10) {
            if let imgPath = Bundle.main.path(forResource: "dir_image", ofType: "png"),
               let nsImg = NSImage(contentsOfFile: imgPath) {
                Image(nsImage: nsImg)
                    .resizable()
                    .scaledToFit()
                    .frame(maxHeight: 90)
                    .overlay(Rectangle().stroke(Color.red.opacity(0.6), lineWidth: 1))
            }
            
            Text("TARGET REQUIRED: SELECT THE PARENT DIRECTORY CONTAINING BOTH GAME VERSIONS")
                .font(.system(size: 13, weight: .bold, design: .monospaced))
                .foregroundColor(Color.red.opacity(0.9))
        }
        .padding(15)
        .background(darkPanel)
        .overlay(Rectangle().stroke(Color.red.opacity(0.5), lineWidth: 2))
        .shadow(color: Color.red.opacity(0.3), radius: 10)
    }
}

struct AlertItem: Identifiable {
    let id = UUID()
    let message: String
}
