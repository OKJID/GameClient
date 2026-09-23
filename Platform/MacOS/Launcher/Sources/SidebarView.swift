import SwiftUI
import AppKit

struct SidebarView: View {
    @ObservedObject var viewModel: LauncherViewModel

    @State private var isScrollRestored = false

    private static let scrollAnchorKey = "SidebarScrollAnchor"
    private let scrollSpace = "sidebarScroll"
    private var profile: GameProfile { viewModel.selectedProfile }
    private var accent: Color { profile.theme.accent }

    private func rememberTopRow(_ offsets: [SettingKey: CGFloat]) {
        guard isScrollRestored else { return }

        let scrolledPast = offsets.filter { $0.value <= 1 }
        guard let topRow = scrolledPast.max(by: { $0.value < $1.value }) else {
            UserDefaults.standard.removeObject(forKey: Self.scrollAnchorKey)
            return
        }

        UserDefaults.standard.set(topRow.key.rawValue, forKey: Self.scrollAnchorKey)
    }

    private func restoreScroll(_ proxy: ScrollViewProxy) {
        let savedRow = UserDefaults.standard.string(forKey: Self.scrollAnchorKey).flatMap(SettingKey.init)

        DispatchQueue.main.async {
            if let savedRow {
                proxy.scrollTo(savedRow, anchor: .top)
            }
            isScrollRestored = true
        }
    }

    var body: some View {
        VStack(spacing: 0) {
            HStack {
                _buildOpenSettingsPageButton()
                Spacer()
                
                Button(action: {
                    viewModel.resetAllSettings()
                }) {
                    HStack(spacing: 4) {
                        Image(systemName: "arrow.counterclockwise")
                            .font(.system(size: 10, weight: .bold))
                        Text(L10n.sidebar.reset)
                            .font(.system(size: 9, weight: .black, design: .monospaced))
                    }
                    .foregroundColor(accent.opacity(0.8))
                }
                .buttonStyle(PlainButtonStyle())
                .onHover { inside in
                    if inside { NSCursor.pointingHand.push() } else { NSCursor.pop() }
                }
            }
            .padding(.bottom, 16)

            ScrollViewReader { proxy in
                ScrollView(.vertical, showsIndicators: false) {
                    VStack(spacing: 16) {
                        ForEach(SettingTopic.allCases, id: \.self) { topic in
                            _buildTopic(topic)
                        }

                        _buildLegendSection()
                    }
                    .padding(.trailing, 4)
                }
                .coordinateSpace(name: scrollSpace)
                .onPreferenceChange(SettingRowOffsetsKey.self, perform: rememberTopRow)
                .onAppear { restoreScroll(proxy) }
            }

            Spacer()
            
            VStack(spacing: 10) {
                _buildLanguageTileButton()
                _buildAboutTileButton()
            }
            .padding(.top, 16)
        }
        .padding(20)
        .frame(width: 300)
        .background(Color.black.opacity(0.45))
        .overlay(
            Rectangle()
                .fill(accent.opacity(0.15))
                .frame(width: 1)
                .frame(maxHeight: .infinity),
            alignment: .leading
        )
    }

    @ViewBuilder
    private func _buildTopic(_ topic: SettingTopic) -> some View {
        let keys = profile.supportedKeys(topic: topic)
        if !keys.isEmpty {
            SettingsSectionHeader(title: topic.title, accent: accent)

            ForEach(keys, id: \.self) { key in
                SettingRow(key: key, viewModel: viewModel)
                    .id(key)
                    .background(_buildOffsetReader(key))
            }
        }
    }

    private func _buildOffsetReader(_ key: SettingKey) -> some View {
        GeometryReader { geometry in
            Color.clear.preference(
                key: SettingRowOffsetsKey.self,
                value: [key: geometry.frame(in: .named(scrollSpace)).minY]
            )
        }
    }

    private func _buildOpenSettingsPageButton() -> some View {
        Button(action: { viewModel.route = .settings }) {
            HStack(spacing: 8) {
                Image(systemName: "slider.horizontal.3")
                    .foregroundColor(accent)

                Text(L10n.settings.title)
                    .font(.system(size: 14, weight: .black, design: .monospaced))
                    .foregroundColor(.white)
                    .lineLimit(1)
                    .minimumScaleFactor(0.7)

                Image(systemName: "arrow.up.left.and.arrow.down.right")
                    .font(.system(size: 10, weight: .bold))
                    .foregroundColor(accent)
            }
            .padding(.horizontal, 10)
            .padding(.vertical, 7)
            .background(accent.opacity(0.1))
            .clipShape(RoundedRectangle(cornerRadius: 6))
            .overlay(
                RoundedRectangle(cornerRadius: 6)
                    .stroke(accent.opacity(0.45), lineWidth: 1)
            )
        }
        .buttonStyle(PlainButtonStyle())
        .help(L10n.settings.openAll)
        .onHover { inside in
            if inside { NSCursor.pointingHand.push() } else { NSCursor.pop() }
        }
    }

    private func _buildLegendSection() -> some View {
        VStack(alignment: .leading, spacing: 8) {
            SettingsSectionHeader(title: L10n.settings.legendSection, accent: accent)
            
            VStack(alignment: .leading, spacing: 8) {
                ForEach(profile.supportedScopes, id: \.self) { scope in
                    _buildLegendItem(scope: scope, desc: scope.description)
                }
            }
            .padding(10)
            .background(Color.white.opacity(0.02))
            .clipShape(RoundedRectangle(cornerRadius: 6))
            .overlay(
                RoundedRectangle(cornerRadius: 6)
                    .stroke(Color.white.opacity(0.04), lineWidth: 1)
            )
        }
        .padding(.top, 8)
    }
    
    private func _buildLegendItem(scope: SettingScope, desc: String) -> some View {
        HStack(alignment: .top, spacing: 8) {
            ScopeChip(scope: scope)
                .frame(width: 80, alignment: .leading)
            
            Text(desc)
                .font(.system(size: 9, design: .monospaced))
                .foregroundColor(.white.opacity(0.6))
                .lineSpacing(2)
                .fixedSize(horizontal: false, vertical: true)
        }
    }

    private func _buildLanguageTileButton() -> some View {
        Menu {
            ForEach(L10n.supportedLanguages, id: \.self) { lang in
                Button(action: {
                    L10n.setCurrent(lang)
                    viewModel.selectedLanguage = lang
                }) {
                    Text(L10n.languageNames[lang] ?? lang)
                }
            }
        } label: {
            HStack {
                Image(systemName: "globe")
                    .font(.system(size: 16))
                    .foregroundColor(accent)
                
                VStack(alignment: .leading, spacing: 2) {
                    Text(L10n.settings.language)
                        .font(.system(size: 9, weight: .bold, design: .monospaced))
                        .foregroundColor(.white.opacity(0.4))
                    Text(L10n.languageNames[viewModel.selectedLanguage] ?? viewModel.selectedLanguage)
                        .font(.system(size: 13, weight: .bold, design: .monospaced))
                        .foregroundColor(.white)
                }
                
                Spacer()
                
                Image(systemName: "chevron.up.chevron.down")
                    .font(.system(size: 12))
                    .foregroundColor(.white.opacity(0.3))
            }
            .padding(.horizontal, 14)
            .padding(.vertical, 10)
            .background(Color.white.opacity(0.03))
            .clipShape(RoundedRectangle(cornerRadius: 6))
            .overlay(RoundedRectangle(cornerRadius: 6).stroke(Color.white.opacity(0.08), lineWidth: 1))
        }
        .menuStyle(BorderlessButtonMenuStyle())
        .onHover { inside in
            if inside { NSCursor.pointingHand.push() } else { NSCursor.pop() }
        }
    }

    private func _buildAboutTileButton() -> some View {
        Button(action: {
            AboutWindowController.show()
        }) {
            HStack {
                Image(systemName: "info.circle")
                    .font(.system(size: 16))
                    .foregroundColor(accent)
                
                VStack(alignment: .leading, spacing: 2) {
                    Text(L10n.settings.about)
                        .font(.system(size: 9, weight: .bold, design: .monospaced))
                        .foregroundColor(.white.opacity(0.4))
                    Text(L10n.settings.aboutButton)
                        .font(.system(size: 13, weight: .bold, design: .monospaced))
                        .foregroundColor(.white)
                }
                
                Spacer()
                
                Image(systemName: "arrow.up.right")
                    .font(.system(size: 12))
                    .foregroundColor(.white.opacity(0.3))
            }
            .padding(.horizontal, 14)
            .padding(.vertical, 10)
            .background(Color.white.opacity(0.03))
            .clipShape(RoundedRectangle(cornerRadius: 6))
            .overlay(RoundedRectangle(cornerRadius: 6).stroke(Color.white.opacity(0.08), lineWidth: 1))
        }
        .buttonStyle(PlainButtonStyle())
        .onHover { inside in
            if inside { NSCursor.pointingHand.push() } else { NSCursor.pop() }
        }
    }
}

// MARK: - Reusable Settings Field with Slider & Text Field Sync & Individual Reset
struct SettingsSliderField: View {
    let title: String
    @Binding var value: Double
    let range: ClosedRange<Double>
    let step: Double
    let format: String
    let defaultValue: Double
    var isDisabled: Bool = false
    let scope: SettingScope
    var showsScope: Bool = true

    @State private var textValue: String = ""
    @State private var pendingTextCommit: DispatchWorkItem?
    @FocusState private var isTextFocused: Bool

    private let textCommitDelay: TimeInterval = 0.4
    private var accent: Color { GameProfile.current.theme.accent }
    private let neonGreen = Color(red: 0.1, green: 0.9, blue: 0.4)

    private var parsedText: Double? {
        Double(textValue.replacingOccurrences(of: ",", with: "."))
    }

    var body: some View {
        VStack(alignment: .leading, spacing: 6) {
            HStack(alignment: .center, spacing: 6) {
                if !title.isEmpty {
                    Text(title)
                        .font(.system(size: 11, weight: .bold, design: .monospaced))
                        .foregroundColor(isDisabled ? .white.opacity(0.2) : .white.opacity(0.7))

                    if showsScope {
                        ScopeChip(scope: scope)
                    }
                }
                Spacer()
                
                if value != defaultValue && !isDisabled {
                    Button(action: {
                        value = defaultValue
                        textValue = String(format: format, defaultValue)
                    }) {
                        Image(systemName: "arrow.counterclockwise")
                            .font(.system(size: 10, weight: .bold))
                            .foregroundColor(accent.opacity(0.8))
                    }
                    .buttonStyle(PlainButtonStyle())
                    .onHover { inside in
                        if inside { NSCursor.pointingHand.push() } else { NSCursor.pop() }
                    }
                }
            }
            
            HStack(spacing: 12) {
                Slider(value: $value, in: range, step: step, onEditingChanged: releaseTextFocus)
                    .accentColor(accent)
                    .disabled(isDisabled)
                    .onChange(of: value, perform: syncText)

                TextField("", text: $textValue)
                .focused($isTextFocused)
                .onSubmit(validateAndCommit)
                .onChange(of: isTextFocused, perform: commitWhenFocusLost)
                .onChange(of: textValue) { _ in scheduleTextCommit() }
                .font(.system(size: 11, weight: .bold, design: .monospaced))
                .foregroundColor(isDisabled ? .white.opacity(0.2) : neonGreen)
                .multilineTextAlignment(.center)
                .frame(width: 50, height: 22)
                .background(Color.black.opacity(0.4))
                .cornerRadius(4)
                .overlay(
                    RoundedRectangle(cornerRadius: 4)
                        .stroke(isDisabled ? Color.white.opacity(0.05) : accent.opacity(0.3), lineWidth: 1)
                )
                .disabled(isDisabled)
                .textFieldStyle(PlainTextFieldStyle())
            }
        }
        .padding(10)
        .background(Color.white.opacity(isDisabled ? 0.01 : 0.02))
        .clipShape(RoundedRectangle(cornerRadius: 6))
        .overlay(
            RoundedRectangle(cornerRadius: 6)
                .stroke(Color.white.opacity(0.04), lineWidth: 1)
        )
        .onAppear {
            textValue = String(format: format, value)
        }
    }
    
    private func releaseTextFocus(isDragging: Bool) {
        guard isDragging else { return }
        isTextFocused = false
    }

    private func commitWhenFocusLost(isFocused: Bool) {
        guard !isFocused else { return }
        validateAndCommit()
    }

    private func syncText(to newValue: Double) {
        guard parsedText != newValue else { return }
        textValue = String(format: format, newValue)
    }

    private func scheduleTextCommit() {
        pendingTextCommit?.cancel()
        let commit = DispatchWorkItem { applyTypedValue() }
        pendingTextCommit = commit
        DispatchQueue.main.asyncAfter(deadline: .now() + textCommitDelay, execute: commit)
    }

    private func applyTypedValue() {
        guard let parsed = parsedText, parsed >= range.lowerBound else { return }
        applyValue(parsed)
    }

    private func validateAndCommit() {
        pendingTextCommit?.cancel()
        if let parsed = parsedText {
            applyValue(parsed)
        }
        textValue = String(format: format, value)
    }

    private func applyValue(_ parsed: Double) {
        let clamped = min(max(parsed, range.lowerBound), range.upperBound)
        let rounded = (clamped / step).rounded() * step
        guard rounded != value else { return }
        value = rounded
    }
}

// MARK: - Settings Scope Support
enum SettingScope: CaseIterable {
    case global
    case online
    case lobbyHost
    case offline

    var title: String {
        switch self {
        case .global: return L10n.settings.scopeGlobal
        case .online: return L10n.settings.scopeOnline
        case .lobbyHost: return L10n.settings.scopeLobbyHost
        case .offline: return L10n.settings.scopeOffline
        }
    }

    var description: String {
        switch self {
        case .global: return L10n.settings.scopeGlobalDesc
        case .online: return L10n.settings.scopeOnlineDesc
        case .lobbyHost: return L10n.settings.scopeLobbyHostDesc
        case .offline: return L10n.settings.scopeOfflineDesc
        }
    }
    
    var color: Color {
        switch self {
        case .global:
            return Color(red: 0.6, green: 0.65, blue: 0.7)
        case .online:
            return Color(red: 0.1, green: 0.5, blue: 1.0)
        case .lobbyHost:
            return Color(red: 0.95, green: 0.6, blue: 0.15)
        case .offline:
            return Color(red: 0.2, green: 0.8, blue: 0.7)
        }
    }
}

struct ScopeChip: View {
    let scope: SettingScope
    
    var body: some View {
        Text(scope.title)
            .font(.system(size: 8, weight: .bold, design: .monospaced))
            .padding(.horizontal, 5)
            .padding(.vertical, 2)
            .foregroundColor(scope.color)
            .background(scope.color.opacity(0.12))
            .cornerRadius(4)
            .overlay(
                RoundedRectangle(cornerRadius: 4)
                    .stroke(scope.color.opacity(0.4), lineWidth: 1)
            )
    }
}
