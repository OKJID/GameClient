import SwiftUI

enum GuideAnchor {
    static let steamTab = "tab.steam"
    static let localTab = "tab.local"
    static let invalidTarget = "local.invalidTarget"
    static let launch = "launch"
    static let update = "update"
    static let mods = "mods"

    static func setting(_ key: SettingKey) -> String {
        "setting.\(key.rawValue)"
    }
}

struct OpenGuideAction {
    let perform: (_ articleID: String, _ source: String) -> Void

    func callAsFunction(article articleID: String, anchor: String) {
        perform(articleID, "hint:\(anchor)")
    }
}

private struct OpenGuideActionKey: EnvironmentKey {
    static let defaultValue = OpenGuideAction { _, _ in }
}

extension EnvironmentValues {
    var openGuide: OpenGuideAction {
        get { self[OpenGuideActionKey.self] }
        set { self[OpenGuideActionKey.self] = newValue }
    }
}

struct GuideHintButton: View {
    let anchor: String

    @ObservedObject private var catalog = GuideCatalog.shared
    @Environment(\.openGuide) private var openGuide
    @State private var isOpen = false

    private func present() {
        isOpen = true
        Analytics.logGuideHintOpened(anchor: anchor)
    }

    private func openArticle(_ article: GuideArticle) {
        isOpen = false
        openGuide(article: article.id, anchor: anchor)
    }

    var body: some View {
        if let hint = catalog.hint(anchor: anchor), let article = catalog.article(id: hint.article) {
            Button(action: present) {
                Image(systemName: "questionmark.circle")
                    .font(.system(size: 12, weight: .semibold))
                    .foregroundColor(.white.opacity(0.55))
                    .contentShape(Circle())
            }
            .buttonStyle(PlainButtonStyle())
            .help(L10n.guide.hintHelp)
            .onHover { inside in
                if inside { NSCursor.pointingHand.push() } else { NSCursor.pop() }
            }
            .popover(isPresented: $isOpen, arrowEdge: .bottom) {
                GuideHintCard(article: article, hint: hint) { openArticle(article) }
            }
        }
    }
}

struct GuideHintCard: View {
    let article: GuideArticle
    let hint: GuideHint
    let onOpenGuide: () -> Void

    private let cardWidth: CGFloat = 320

    private var title: String { article.text(for: L10n.current)?.title ?? article.id }

    var body: some View {
        VStack(alignment: .leading, spacing: 10) {
            Text(title)
                .font(.system(size: 13, weight: .bold, design: .monospaced))
                .fixedSize(horizontal: false, vertical: true)

            if let text = hint.text(for: L10n.current) {
                Text(text)
                    .font(.system(size: 11, design: .monospaced))
                    .foregroundColor(.secondary)
                    .fixedSize(horizontal: false, vertical: true)
            }

            GuideMoreButton(action: onOpenGuide)
        }
        .padding(16)
        .frame(width: cardWidth, alignment: .leading)
    }
}

struct GuideHintNote: View {
    let anchor: String
    let color: Color

    @ObservedObject private var catalog = GuideCatalog.shared
    @Environment(\.openGuide) private var openGuide

    var body: some View {
        if let hint = catalog.hint(anchor: anchor), let text = hint.text(for: L10n.current) {
            VStack(alignment: .leading, spacing: 6) {
                Text(text)
                    .font(.system(size: 11, design: .monospaced))
                    .foregroundColor(color)
                    .fixedSize(horizontal: false, vertical: true)

                GuideMoreButton { openGuide(article: hint.article, anchor: anchor) }
            }
        }
    }
}

struct GuideMoreButton: View {
    let action: () -> Void

    var body: some View {
        Button(action: action) {
            HStack(spacing: 6) {
                Image(systemName: "book")
                Text(L10n.guide.more)
                Image(systemName: "arrow.right")
            }
            .font(.system(size: 11, weight: .bold, design: .monospaced))
            .foregroundColor(GameProfile.current.theme.accent)
            .contentShape(Rectangle())
        }
        .buttonStyle(PlainButtonStyle())
        .onHover { inside in
            if inside { NSCursor.pointingHand.push() } else { NSCursor.pop() }
        }
    }
}
