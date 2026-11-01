import SwiftUI
import WebKit

struct GuideArticleWebView: NSViewRepresentable {
    let html: String
    let accent: Color
    let articleID: String
    let onLink: (LauncherLink) -> Void

    func makeCoordinator() -> GuideWebNavigator {
        GuideWebNavigator(articleID: articleID, onLink: onLink)
    }

    func makeNSView(context: Context) -> WKWebView {
        let configuration = WKWebViewConfiguration()
        configuration.defaultWebpagePreferences.allowsContentJavaScript = false

        let webView = WKWebView(frame: .zero, configuration: configuration)
        webView.navigationDelegate = context.coordinator
        webView.setValue(false, forKey: "drawsBackground")

        return webView
    }

    func updateNSView(_ webView: WKWebView, context: Context) {
        context.coordinator.articleID = articleID
        context.coordinator.onLink = onLink

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
          html, body { margin: 0; padding: 0; background: transparent; color-scheme: dark; }
          body {
            padding: 4px 18px 24px 4px;
            font-family: ui-monospace, Menlo, monospace; font-size: 13px; line-height: 1.6;
            color: rgba(255,255,255,0.82); -webkit-font-smoothing: antialiased;
          }
          h2, h3 { color: #ffffff; margin: 22px 0 8px; }
          h2 { font-size: 15px; }
          h3 { font-size: 13px; }
          p { margin: 0 0 10px; }
          ul, ol { margin: 0 0 12px; padding-left: 22px; }
          li { margin: 4px 0; }
          a { color: \(accentHex()); font-weight: 700; text-decoration: underline; }
          b, strong { color: #ffffff; }
          code, kbd {
            padding: 1px 5px; border-radius: 4px;
            background: rgba(255,255,255,0.08); border: 1px solid rgba(255,255,255,0.14);
            color: #ffffff; font-family: inherit; font-size: 12px;
          }
          .note {
            margin: 12px 0; padding: 10px 12px; border-radius: 6px;
            background: rgba(255,255,255,0.04); border-left: 3px solid \(accentHex());
          }
          img { max-width: 100%; border-radius: 6px; }
        </style></head>
        <body>\(html)</body></html>
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

final class GuideWebNavigator: NSObject, WKNavigationDelegate {
    private static let externalSchemes: Set<String> = ["https", "tg", "discord", "mailto"]

    var articleID: String
    var onLink: (LauncherLink) -> Void
    var renderedDocument: String = ""

    init(articleID: String, onLink: @escaping (LauncherLink) -> Void) {
        self.articleID = articleID
        self.onLink = onLink
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
        follow(url)
    }

    private func follow(_ url: URL) {
        if let link = LauncherLink(url: url) {
            Analytics.logGuideLinkFollowed(articleID: articleID, url: url)
            onLink(link)
            return
        }

        guard let scheme = url.scheme?.lowercased(), Self.externalSchemes.contains(scheme) else { return }

        Analytics.logGuideLinkFollowed(articleID: articleID, url: url)
        NSWorkspace.shared.open(url)
    }
}
