import SwiftUI
import AppKit

struct AboutView: View {
    private let neonBlue = Color(red: 0.1, green: 0.5, blue: 1.0)
    private let version = Bundle.main.infoDictionary?["CFBundleShortVersionString"] as? String ?? "1.0"

    var body: some View {
        VStack(spacing: 20) {
            _buildLogo()
            _buildTitle()
            _buildCredits()
            Divider().background(Color.white.opacity(0.2))
            _buildLinks()
            _buildLegal()
        }
        .padding(30)
        .frame(width: 420, height: 480)
        .background(Color(white: 0.1))
    }

    private func _buildLogo() -> some View {
        Group {
            if let imgPath = Bundle.main.path(forResource: "AppIcon", ofType: "png"),
               let nsImg = NSImage(contentsOfFile: imgPath) {
                Image(nsImage: nsImg)
                    .resizable()
                    .scaledToFit()
                    .frame(width: 80, height: 80)
                    .clipShape(RoundedRectangle(cornerRadius: 16))
                    .shadow(color: neonBlue.opacity(0.4), radius: 8)
            } else {
                Image(systemName: "gamecontroller.fill")
                    .font(.system(size: 50))
                    .foregroundColor(neonBlue)
            }
        }
    }

    private func _buildTitle() -> some View {
        VStack(spacing: 4) {
            Text("Generals Online")
                .font(.system(size: 22, weight: .bold))
                .foregroundColor(.white)

            Text("macOS Native Port")
                .font(.system(size: 14, weight: .medium, design: .monospaced))
                .foregroundColor(neonBlue)

            Text("Version \(version)")
                .font(.system(size: 12, design: .monospaced))
                .foregroundColor(.white.opacity(0.5))
        }
    }

    private func _buildCredits() -> some View {
        VStack(spacing: 8) {
            HStack(spacing: 12) {
                if let imgPath = Bundle.main.path(forResource: "author_logo", ofType: "png"),
                   let nsImg = NSImage(contentsOfFile: imgPath) {
                    Image(nsImage: nsImg)
                        .resizable()
                        .scaledToFit()
                        .frame(width: 36, height: 36)
                        .clipShape(Circle())
                        .overlay(Circle().stroke(neonBlue.opacity(0.5), lineWidth: 1))
                }

                VStack(alignment: .leading, spacing: 2) {
                    Text("macOS Port by Dima Ok (OKJI)")
                        .font(.system(size: 13, weight: .semibold))
                        .foregroundColor(.white)

                    Text("Metal Renderer • Apple Silicon")
                        .font(.system(size: 11, design: .monospaced))
                        .foregroundColor(.white.opacity(0.5))
                }
            }

            Text("Built on top of the EA GPLv3 open-source release")
                .font(.system(size: 11))
                .foregroundColor(.white.opacity(0.4))
                .multilineTextAlignment(.center)
        }
    }

    private func _buildLinks() -> some View {
        HStack(spacing: 20) {
            _buildLink(title: "Website", url: "https://general-online-zh.web.app")
            _buildLink(title: "Telegram", url: "https://t.me/GeneralsOnlineMacOSChannel")
            _buildLink(title: "GitHub", url: "https://github.com/GeneralsOnlineDevelopmentTeam/GameClient")
        }
    }

    private func _buildLink(title: String, url: String) -> some View {
        Button(action: {
            if let link = URL(string: url) {
                NSWorkspace.shared.open(link)
            }
        }) {
            Text(title)
                .font(.system(size: 12, weight: .medium, design: .monospaced))
                .foregroundColor(neonBlue)
                .underline()
        }
        .buttonStyle(PlainButtonStyle())
        .onHover { inside in
            if inside { NSCursor.pointingHand.push() } else { NSCursor.pop() }
        }
    }

    private func _buildLegal() -> some View {
        Text("C&C: Generals Zero Hour™ is a trademark of Electronic Arts.\nGame engine source code licensed under GPLv3.")
            .font(.system(size: 10))
            .foregroundColor(.white.opacity(0.3))
            .multilineTextAlignment(.center)
            .lineSpacing(2)
    }
}

class AboutWindowController {
    private static var window: NSWindow?

    static func show() {
        if let existing = window {
            existing.makeKeyAndOrderFront(nil)
            NSApp.activate(ignoringOtherApps: true)
            return
        }

        let aboutView = NSHostingView(rootView: AboutView())
        let win = NSWindow(
            contentRect: NSRect(x: 0, y: 0, width: 420, height: 480),
            styleMask: [.titled, .closable],
            backing: .buffered,
            defer: false
        )
        win.contentView = aboutView
        win.title = "About Generals Online"
        win.center()
        win.isReleasedWhenClosed = false
        win.makeKeyAndOrderFront(nil)
        NSApp.activate(ignoringOtherApps: true)

        window = win
    }
}
