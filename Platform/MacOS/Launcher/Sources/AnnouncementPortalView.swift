import SwiftUI
import WebKit

struct AnnouncementPortalView: View {
    private static let tickInterval: TimeInterval = 0.25

    let items: [Announcement]
    let accent: Color

    @State private var index: Int = 0
    @State private var elapsed: TimeInterval = 0
    @State private var isPaused: Bool = false
    @State private var shimmer: CGFloat = -0.4

    private let tick = Timer
        .publish(every: AnnouncementPortalView.tickInterval, on: .main, in: .common)
        .autoconnect()

    private var current: Announcement? {
        guard !items.isEmpty else { return nil }
        return items[min(index, items.count - 1)]
    }

    private func advance() {
        guard items.count > 1, !isPaused else { return }
        guard let current else { return }

        elapsed += Self.tickInterval
        guard elapsed >= current.duration else { return }

        elapsed = 0
        withAnimation(.easeInOut(duration: 0.35)) {
            index = (index + 1) % items.count
        }
    }

    private func startShimmer() {
        shimmer = -0.4
        withAnimation(.linear(duration: 3.2).repeatForever(autoreverses: false)) {
            shimmer = 1.4
        }
    }

    var body: some View {
        _buildStage()
            .frame(height: 82)
            .frame(maxWidth: .infinity)
            .background(Color.black.opacity(0.5))
            .background(accent.opacity(0.12))
            .clipShape(RoundedRectangle(cornerRadius: 8))
            .overlay(_buildShimmerBorder())
            .overlay(_buildIndicators(), alignment: .bottom)
            .shadow(color: accent.opacity(0.3), radius: 7)
            .onReceive(tick) { _ in advance() }
            .onHover { inside in isPaused = inside }
            .onAppear(perform: startShimmer)
    }

    @ViewBuilder
    private func _buildStage() -> some View {
        if let current, let html = current.html(for: L10n.current) {
            AnnouncementWebView(html: html, accent: accent, announcementId: current.id)
                .id(current.id)
                .transition(.opacity)
                .onAppear { Analytics.logAnnouncementShown(id: current.id) }
                .padding(.horizontal, 12)
                .padding(.vertical, 8)
        } else {
            _buildFallback()
        }
    }

    private func _buildShimmerBorder() -> some View {
        RoundedRectangle(cornerRadius: 8)
            .strokeBorder(
                LinearGradient(
                    stops: [
                        .init(color: accent.opacity(0.45), location: 0),
                        .init(color: Color.white.opacity(0.95), location: 0.5),
                        .init(color: accent.opacity(0.45), location: 1)
                    ],
                    startPoint: UnitPoint(x: shimmer - 0.4, y: 0),
                    endPoint: UnitPoint(x: shimmer + 0.4, y: 1)
                ),
                lineWidth: 1.5
            )
            .allowsHitTesting(false)
    }

    private func _buildFallback() -> some View {
        VStack(spacing: 4) {
            Text(L10n.portal.onlineNotice)
                .font(.system(size: 12, weight: .bold, design: .monospaced))
                .foregroundColor(.white.opacity(0.85))
                .multilineTextAlignment(.center)

            HStack(spacing: 8) {
                Text(L10n.portal.findOpponents)
                    .font(.system(size: 12, weight: .medium, design: .monospaced))
                    .foregroundColor(.white.opacity(0.7))

                _buildFallbackLink(title: "Telegram", url: CommunityLinks.telegram)
                Text("·").foregroundColor(.white.opacity(0.4)).font(.system(size: 13))
                _buildFallbackLink(title: "Discord", url: CommunityLinks.discord)
            }
        }
        .padding(.horizontal, 12)
        .padding(.vertical, 8)
    }

    private func _buildFallbackLink(title: String, url: String) -> some View {
        Button(action: {
            Analytics.logLinkOpened(target: title.lowercased(), location: "portal_fallback")
            if let link = URL(string: url) { NSWorkspace.shared.open(link) }
        }) {
            Text(title)
                .font(.system(size: 12, weight: .bold, design: .monospaced))
                .foregroundColor(accent)
                .underline()
        }
        .buttonStyle(PlainButtonStyle())
        .onHover { inside in
            if inside { NSCursor.pointingHand.push() } else { NSCursor.pop() }
        }
    }

    @ViewBuilder
    private func _buildIndicators() -> some View {
        if items.count > 1 {
            HStack(spacing: 5) {
                ForEach(Array(items.enumerated()), id: \.element.id) { position, _ in
                    Circle()
                        .fill(position == index ? accent.opacity(0.9) : Color.white.opacity(0.25))
                        .frame(width: 4, height: 4)
                }
            }
            .padding(.bottom, 4)
            .allowsHitTesting(false)
        }
    }
}

struct AnnouncementWebView: NSViewRepresentable {
    let html: String
    let accent: Color
    let announcementId: String

    func makeCoordinator() -> AnnouncementWebNavigator {
        AnnouncementWebNavigator(announcementId: announcementId)
    }

    func makeNSView(context: Context) -> WKWebView {
        let configuration = WKWebViewConfiguration()
        configuration.defaultWebpagePreferences.allowsContentJavaScript = false

        let webView = WKWebView(frame: .zero, configuration: configuration)
        webView.navigationDelegate = context.coordinator
        webView.setValue(false, forKey: "drawsBackground")
        webView.loadHTMLString(document(), baseURL: nil)

        return webView
    }

    func updateNSView(_ webView: WKWebView, context: Context) {
        let rendered = document()
        guard rendered != context.coordinator.renderedDocument else { return }

        context.coordinator.renderedDocument = rendered
        webView.loadHTMLString(rendered, baseURL: nil)
    }

    private func document() -> String {
        """
        <!DOCTYPE html>
        <html><head><meta charset="utf-8">
        <meta http-equiv="Content-Security-Policy"
              content="default-src 'none'; img-src data: https:; style-src 'unsafe-inline'; font-src data:;">
        <style>
          html, body { margin: 0; padding: 0; background: transparent; height: 100%; overflow: hidden; }
          body {
            display: flex; align-items: center; justify-content: center; text-align: center;
            font-family: ui-monospace, Menlo, monospace; font-size: 13px; line-height: 1.5;
            color: rgba(255,255,255,0.88); -webkit-font-smoothing: antialiased;
          }
          a { color: \(accentHex()); font-weight: 700; text-decoration: underline; }
          b, strong { color: #ffffff; }
          img { max-height: 40px; vertical-align: middle; }
        </style></head>
        <body><div>\(html)</div></body></html>
        """
    }

    private func accentHex() -> String {
        guard let rgb = NSColor(accent).usingColorSpace(.sRGB) else { return "#f2c14b" }

        let channels = [rgb.redComponent, rgb.greenComponent, rgb.blueComponent]
            .map { String(format: "%02x", Int(($0 * 255).rounded())) }
            .joined()

        return "#\(channels)"
    }
}

final class AnnouncementWebNavigator: NSObject, WKNavigationDelegate {
    private static let allowedSchemes: Set<String> = ["https", "tg", "discord", "mailto"]

    let announcementId: String

    var renderedDocument: String = ""

    init(announcementId: String) {
        self.announcementId = announcementId
    }

    func webView(
        _ webView: WKWebView,
        decidePolicyFor navigationAction: WKNavigationAction,
        decisionHandler: @escaping (WKNavigationActionPolicy) -> Void
    ) {
        guard let url = navigationAction.request.url else {
            decisionHandler(.cancel)
            return
        }

        if url.scheme == "about" {
            decisionHandler(.allow)
            return
        }

        decisionHandler(.cancel)

        guard let scheme = url.scheme?.lowercased(), Self.allowedSchemes.contains(scheme) else { return }

        Analytics.logAnnouncementClicked(id: announcementId, url: url)
        NSWorkspace.shared.open(url)
    }
}
