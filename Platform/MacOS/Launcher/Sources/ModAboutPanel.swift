import SwiftUI

struct ModAboutButton: View {
    let profile: GameProfile
    let color: Color
    let onAuthorRequest: () -> Void

    @ObservedObject private var catalog = ModAboutCatalog.shared
    @State private var isOpen = false

    private func open(_ url: URL, target: String) {
        Analytics.logLinkOpened(target: target, location: "mod_about")
        isOpen = false
        NSWorkspace.shared.open(url)
    }

    private func requestAsAuthor() {
        isOpen = false
        DispatchQueue.main.async(execute: onAuthorRequest)
    }

    var body: some View {
        Button(action: { isOpen.toggle() }) {
            HStack(spacing: 5) {
                Image(systemName: "info.circle")
                Text(L10n.mod.about.button)
            }
            .font(.system(size: 11, weight: .bold, design: .monospaced))
            .foregroundColor(color)
            .padding(.horizontal, 14)
            .padding(.vertical, 6)
            .background(RoundedRectangle(cornerRadius: 4).fill(color.opacity(0.18)))
            .overlay(RoundedRectangle(cornerRadius: 4).stroke(color.opacity(0.85), lineWidth: 1))
            .contentShape(RoundedRectangle(cornerRadius: 4))
        }
        .buttonStyle(PlainButtonStyle())
        .help(L10n.mod.about.help)
        .onHover { inside in
            if inside { NSCursor.pointingHand.push() } else { NSCursor.pop() }
        }
        .popover(isPresented: $isOpen, arrowEdge: .top) {
            ModAboutPanel(
                profile: profile,
                about: catalog.about(for: profile.id),
                onOpenLink: open,
                onAuthorRequest: requestAsAuthor
            )
        }
    }
}

struct ModAboutPanel: View {
    let profile: GameProfile
    let about: ModAbout?
    let onOpenLink: (URL, String) -> Void
    let onAuthorRequest: () -> Void

    private let panelWidth: CGFloat = 320
    private let medallionSize: CGFloat = 56
    private let buttonFillOpacity: Double = 0.06
    private let buttonStrokeOpacity: Double = 0.2
    private let approvedColor = Color(red: 0.1, green: 0.75, blue: 0.4)

    private var description: String? { about?.description(for: L10n.current) }
    private var includes: [ModInclude] { about?.includes ?? [] }
    private var links: [ModLink] { about?.links ?? [] }

    var body: some View {
        VStack(alignment: .leading, spacing: 10) {
            _buildHeader()
            _buildAuthorRequest()
            _buildAttribution()

            if let description {
                Text(description)
                    .font(.system(size: 11, design: .monospaced))
                    .fixedSize(horizontal: false, vertical: true)
            }

            if !includes.isEmpty {
                Divider()
                _buildIncludes()
            }

            if !links.isEmpty {
                Divider()
                _buildLinks()
            }
        }
        .padding(16)
        .frame(width: panelWidth, alignment: .leading)
    }

    private func _buildHeader() -> some View {
        HStack(alignment: .center, spacing: 12) {
            ModArtworkImage(path: profile.mod?.medallionPath, fallback: ModArtwork.defaultMedallion, contentMode: .fit)
                .frame(width: medallionSize, height: medallionSize)

            VStack(alignment: .leading, spacing: 4) {
                Text(profile.displayName)
                    .font(.system(size: 13, weight: .bold, design: .monospaced))

                if let version = about?.version {
                    _buildDetail(label: L10n.mod.about.version, value: version)
                }

                if let authors = about?.authors {
                    _buildDetail(label: L10n.mod.about.authors, value: authors)
                }
            }
        }
    }

    private func _buildDetail(label: String, value: String) -> some View {
        Text("\(label): \(value)")
            .font(.system(size: 10, design: .monospaced))
            .foregroundColor(.secondary)
            .fixedSize(horizontal: false, vertical: true)
    }

    @ViewBuilder
    private func _buildAttribution() -> some View {
        switch about?.attribution {
        case nil:
            _buildNotice(L10n.mod.about.empty, icon: "info.circle", color: .secondary)
        case .unknown:
            _buildNotice(L10n.mod.about.unknownAuthor, icon: "questionmark.circle", color: .orange)
        case .approved:
            _buildNotice(L10n.mod.about.approved, icon: "checkmark.seal.fill", color: approvedColor)
        case .credited:
            EmptyView()
        }
    }

    private func _buildNotice(_ text: String, icon: String, color: Color) -> some View {
        HStack(alignment: .top, spacing: 6) {
            Image(systemName: icon)
            Text(text)
                .fixedSize(horizontal: false, vertical: true)
        }
        .font(.system(size: 11, weight: .medium, design: .monospaced))
        .foregroundColor(color)
    }

    private func _buildIncludes() -> some View {
        VStack(alignment: .leading, spacing: 6) {
            Text(L10n.mod.about.includes)
                .font(.system(size: 10, weight: .bold, design: .monospaced))
                .foregroundColor(.secondary)

            ForEach(includes) { include in
                _buildInclude(include)
            }
        }
    }

    private func _buildInclude(_ include: ModInclude) -> some View {
        HStack(alignment: .top, spacing: 8) {
            VStack(alignment: .leading, spacing: 2) {
                Text(include.name)
                    .font(.system(size: 11, weight: .bold, design: .monospaced))
                    .fixedSize(horizontal: false, vertical: true)

                if let authors = include.authors {
                    Text(authors)
                        .font(.system(size: 10, design: .monospaced))
                        .foregroundColor(.secondary)
                }
            }

            Spacer(minLength: 0)

            if let url = include.url {
                _buildLinkIcon(url: url, target: "include")
            }
        }
    }

    private func _buildLinks() -> some View {
        VStack(alignment: .leading, spacing: 6) {
            ForEach(links) { link in
                _buildLinkRow(link)
            }
        }
    }

    private func _buildLinkRow(_ link: ModLink) -> some View {
        Button(action: { onOpenLink(link.url, link.kind?.rawValue ?? "other") }) {
            HStack(spacing: 10) {
                Image(systemName: link.icon)
                    .frame(width: 18)

                Text(link.title)
                    .font(.system(size: 12, weight: .bold, design: .monospaced))

                Spacer()

                Image(systemName: "arrow.up.right")
                    .font(.system(size: 10))
                    .foregroundColor(.secondary)
            }
            .padding(.horizontal, 10)
            .padding(.vertical, 7)
            .background(_buildButtonShape())
            .contentShape(RoundedRectangle(cornerRadius: 6))
        }
        .buttonStyle(PlainButtonStyle())
        .help(link.url.absoluteString)
        .onHover(perform: _updateCursor)
    }

    private func _buildAuthorRequest() -> some View {
        HStack(spacing: 10) {
            Text(L10n.mod.author.question)
                .font(.system(size: 10, weight: .bold, design: .monospaced))
                .fixedSize()

            Button(action: onAuthorRequest) {
                HStack(spacing: 6) {
                    Image(systemName: "pencil.and.outline")
                    Text(L10n.mod.author.button)
                        .fixedSize(horizontal: false, vertical: true)
                    Spacer(minLength: 0)
                }
                .font(.system(size: 10, weight: .medium, design: .monospaced))
                .padding(.horizontal, 8)
                .padding(.vertical, 5)
                .background(_buildButtonShape())
                .contentShape(RoundedRectangle(cornerRadius: 6))
            }
            .buttonStyle(PlainButtonStyle())
            .onHover(perform: _updateCursor)
        }
    }

    private func _buildButtonShape() -> some View {
        RoundedRectangle(cornerRadius: 6)
            .fill(Color.primary.opacity(buttonFillOpacity))
            .overlay(RoundedRectangle(cornerRadius: 6).stroke(Color.primary.opacity(buttonStrokeOpacity), lineWidth: 1))
    }

    private func _updateCursor(_ inside: Bool) {
        if inside { NSCursor.pointingHand.push() } else { NSCursor.pop() }
    }

    private func _buildLinkIcon(url: URL, target: String) -> some View {
        Button(action: { onOpenLink(url, target) }) {
            Image(systemName: "arrow.up.right.square")
                .font(.system(size: 12))
        }
        .buttonStyle(PlainButtonStyle())
        .help(url.absoluteString)
    }
}
