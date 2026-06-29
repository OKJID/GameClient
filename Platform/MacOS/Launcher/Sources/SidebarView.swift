import SwiftUI
import AppKit

struct SidebarView: View {
    @ObservedObject var viewModel: LauncherViewModel

    private let neonBlue = Color(red: 0.1, green: 0.5, blue: 1.0)
    private let neonGreen = Color(red: 0.1, green: 0.9, blue: 0.4)

    var body: some View {
        VStack(spacing: 0) {
            // Title & Global Reset
            HStack {
                Image(systemName: "slider.horizontal.3")
                    .foregroundColor(neonBlue)
                Text(L10n.settings.title)
                    .font(.system(size: 16, weight: .black, design: .monospaced))
                    .foregroundColor(.white)
                Spacer()
                
                Button(action: {
                    viewModel.resetAllSettings()
                }) {
                    HStack(spacing: 4) {
                        Image(systemName: "arrow.counterclockwise")
                            .font(.system(size: 10, weight: .bold))
                        Text("RESET")
                            .font(.system(size: 9, weight: .black, design: .monospaced))
                    }
                    .foregroundColor(neonBlue.opacity(0.8))
                }
                .buttonStyle(PlainButtonStyle())
                .onHover { inside in
                    if inside { NSCursor.pointingHand.push() } else { NSCursor.pop() }
                }
            }
            .padding(.bottom, 16)
            
            // Scrolling Settings List
            ScrollView(.vertical, showsIndicators: false) {
                VStack(spacing: 16) {

                    // --- INTERFACE SECTION ---
                    _buildSectionHeader(title: L10n.settings.interfaceSection)
                    
                    _buildGameLanguagePicker()
                    
                    _buildSidebarSettingToggle(
                        title: L10n.settings.windowedEdgeScroll,
                        description: L10n.settings.windowedEdgeScrollDesc,
                        isOn: $viewModel.isWindowedEdgeScrollEnabled,
                        scope: .global
                    )

                    _buildSidebarSettingToggle(
                        title: L10n.settings.showHotkeyLabels,
                        description: L10n.settings.showHotkeyLabelsDesc,
                        isOn: $viewModel.showHotkeyLabels,
                        scope: .global
                    )

                    // --- CAMERA SECTION ---
                    _buildSectionHeader(title: L10n.settings.cameraSection)
                    
                    SettingsSliderField(
                        title: L10n.settings.cameraMaxHeight,
                        value: $viewModel.cameraMaxHeight,
                        range: SettingsDefaults.cameraMaxHeightRange,
                        step: SettingsDefaults.cameraMaxHeightStep,
                        format: SettingsDefaults.cameraMaxHeightFormat,
                        defaultValue: SettingsDefaults.cameraMaxHeight,
                        scope: .lobbyHost
                    )
                    
                    SettingsSliderField(
                        title: L10n.settings.cameraMinHeight,
                        value: $viewModel.cameraMinHeight,
                        range: SettingsDefaults.cameraMinHeightRange,
                        step: SettingsDefaults.cameraMinHeightStep,
                        format: SettingsDefaults.cameraMinHeightFormat,
                        defaultValue: SettingsDefaults.cameraMinHeight,
                        scope: .global
                    )
                    
                    SettingsSliderField(
                        title: L10n.settings.cameraSpeed,
                        value: $viewModel.cameraMoveSpeed,
                        range: SettingsDefaults.cameraMoveSpeedRange,
                        step: SettingsDefaults.cameraMoveSpeedStep,
                        format: SettingsDefaults.cameraMoveSpeedFormat,
                        defaultValue: SettingsDefaults.cameraMoveSpeed,
                        scope: .global
                    )
                    // --- PERFORMANCE SECTION ---
                    _buildSectionHeader(title: L10n.settings.fpsSection)
                    
                    _buildSidebarSettingToggle(
                        title: L10n.settings.limitFps,
                        description: "",
                        isOn: $viewModel.limitFramerate,
                        scope: .global
                    )
                    
                    SettingsSliderField(
                        title: "",
                        value: $viewModel.fpsLimit,
                        range: SettingsDefaults.fpsLimitRange,
                        step: SettingsDefaults.fpsLimitStep,
                        format: SettingsDefaults.fpsLimitFormat,
                        defaultValue: SettingsDefaults.fpsLimit,
                        isDisabled: !viewModel.limitFramerate,
                        scope: .global
                    )
                    
                    _buildSidebarSettingToggle(
                        title: L10n.settings.statsOverlay,
                        description: "",
                        isOn: $viewModel.statsOverlay,
                        scope: .global
                    )
                    
                    // --- NETWORK & DEBUG SECTION ---
                    _buildSectionHeader(title: L10n.settings.networkSection)
                    
                    _buildSidebarSettingToggle(
                        title: L10n.settings.altEndpoint,
                        description: "",
                        isOn: $viewModel.useAlternativeEndpoint,
                        scope: .online
                    )
                    
                    _buildSidebarSettingToggle(
                        title: L10n.settings.verboseLogging,
                        description: "",
                        isOn: $viewModel.verboseLogging,
                        scope: .global
                    )
                    
                    // --- LEGEND SECTION ---
                    _buildLegendSection()
                }
                .padding(.trailing, 4)
            }
            
            Spacer()
            
            // Bottom Tile Buttons
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
                .fill(neonBlue.opacity(0.15))
                .frame(width: 1)
                .frame(maxHeight: .infinity),
            alignment: .leading
        )
    }

    private func _buildSectionHeader(title: String) -> some View {
        HStack(spacing: 8) {
            Text(title.uppercased())
                .font(.system(size: 10, weight: .black, design: .monospaced))
                .foregroundColor(neonBlue.opacity(0.8))
            
            VStack {
                Divider()
                    .background(neonBlue.opacity(0.15))
            }
        }
        .padding(.top, 8)
        .padding(.bottom, 4)
    }

    private func _buildSidebarSettingToggle(title: String, description: String, isOn: Binding<Bool>, scope: SettingScope) -> some View {
        Button(action: { isOn.wrappedValue.toggle() }) {
            HStack(alignment: .top, spacing: 12) {
                Image(systemName: isOn.wrappedValue ? "checkmark.square.fill" : "square")
                    .font(.system(size: 18))
                    .foregroundColor(isOn.wrappedValue ? neonGreen : .white.opacity(0.3))
                    .padding(.top, 2)
                
                VStack(alignment: .leading, spacing: 4) {
                    HStack(alignment: .center, spacing: 6) {
                        Text(title)
                            .font(.system(size: 13, weight: .bold, design: .monospaced))
                            .foregroundColor(.white)
                            .multilineTextAlignment(.leading)
                        
                        ScopeChip(scope: scope)
                    }
                    
                    if !description.isEmpty {
                        Text(description)
                            .font(.system(size: 10, design: .monospaced))
                            .foregroundColor(.white.opacity(0.5))
                            .multilineTextAlignment(.leading)
                            .fixedSize(horizontal: false, vertical: true)
                    }
                }
                Spacer()
            }
            .padding(12)
            .background(Color.white.opacity(isOn.wrappedValue ? 0.05 : 0.02))
            .clipShape(RoundedRectangle(cornerRadius: 6))
            .overlay(
                RoundedRectangle(cornerRadius: 6)
                    .stroke(isOn.wrappedValue ? neonBlue.opacity(0.4) : Color.white.opacity(0.05), lineWidth: 1)
            )
        }
        .buttonStyle(PlainButtonStyle())
        .onHover { inside in
            if inside { NSCursor.pointingHand.push() } else { NSCursor.pop() }
        }
    }

    private func _buildGameLanguagePicker() -> some View {
        let zhDir: URL? = {
            switch viewModel.activeTab {
            case .steam: return viewModel.steamCMD.assetsDir
            case .local: return viewModel.zhDirectoryURL
            }
        }()
        let installed = zhDir.map { AssetPatcher.installedLanguages(at: $0) } ?? AssetPatcher.availableLanguages

        return VStack(alignment: .leading, spacing: 6) {
            HStack(spacing: 8) {
                Text(L10n.settings.gameLanguage)
                    .font(.system(size: 12, weight: .medium, design: .monospaced))
                    .foregroundColor(.white.opacity(0.9))
                ScopeChip(scope: .global)
            }

            Picker("", selection: $viewModel.gameLanguage) {
                ForEach(installed, id: \.self) { lang in
                    Text(lang.capitalized).tag(lang)
                }
            }
            .labelsHidden()
            .pickerStyle(.menu)

            Text(L10n.settings.gameLanguageRestart)
                .font(.system(size: 10, design: .monospaced))
                .foregroundColor(.white.opacity(0.5))
        }
        .padding(12)
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(Color.white.opacity(0.02))
        .clipShape(RoundedRectangle(cornerRadius: 6))
        .overlay(
            RoundedRectangle(cornerRadius: 6)
                .stroke(Color.white.opacity(0.05), lineWidth: 1)
        )
    }

    private func _buildLegendSection() -> some View {
        VStack(alignment: .leading, spacing: 8) {
            _buildSectionHeader(title: L10n.settings.legendSection)
            
            VStack(alignment: .leading, spacing: 8) {
                _buildLegendItem(scope: .global, desc: L10n.settings.scopeGlobalDesc)
                _buildLegendItem(scope: .online, desc: L10n.settings.scopeOnlineDesc)
                _buildLegendItem(scope: .lobbyHost, desc: L10n.settings.scopeLobbyHostDesc)
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
                    .foregroundColor(neonBlue)
                
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
                    .foregroundColor(neonBlue)
                
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
    
    @State private var textValue: String = ""
    @State private var isEditingText: Bool = false
    
    private let neonBlue = Color(red: 0.1, green: 0.5, blue: 1.0)
    private let neonGreen = Color(red: 0.1, green: 0.9, blue: 0.4)
    
    var body: some View {
        VStack(alignment: .leading, spacing: 6) {
            HStack(alignment: .center, spacing: 6) {
                if !title.isEmpty {
                    Text(title)
                        .font(.system(size: 11, weight: .bold, design: .monospaced))
                        .foregroundColor(isDisabled ? .white.opacity(0.2) : .white.opacity(0.7))
                    
                    ScopeChip(scope: scope)
                }
                Spacer()
                
                // Per-field reset button
                if value != defaultValue && !isDisabled {
                    Button(action: {
                        value = defaultValue
                        textValue = String(format: format, defaultValue)
                    }) {
                        Image(systemName: "arrow.counterclockwise")
                            .font(.system(size: 10, weight: .bold))
                            .foregroundColor(neonBlue.opacity(0.8))
                    }
                    .buttonStyle(PlainButtonStyle())
                    .onHover { inside in
                        if inside { NSCursor.pointingHand.push() } else { NSCursor.pop() }
                    }
                }
            }
            
            HStack(spacing: 12) {
                Slider(value: $value, in: range, step: step)
                    .accentColor(neonBlue)
                    .disabled(isDisabled)
                    .onChange(of: value) { newValue in
                        if !isEditingText {
                            textValue = String(format: format, newValue)
                        }
                    }
                
                TextField("", text: $textValue, onEditingChanged: { editing in
                    isEditingText = editing
                    if !editing {
                        validateAndCommit()
                    }
                }, onCommit: {
                    validateAndCommit()
                })
                .font(.system(size: 11, weight: .bold, design: .monospaced))
                .foregroundColor(isDisabled ? .white.opacity(0.2) : neonGreen)
                .multilineTextAlignment(.center)
                .frame(width: 50, height: 22)
                .background(Color.black.opacity(0.4))
                .cornerRadius(4)
                .overlay(
                    RoundedRectangle(cornerRadius: 4)
                        .stroke(isDisabled ? Color.white.opacity(0.05) : neonBlue.opacity(0.3), lineWidth: 1)
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
    
    private func validateAndCommit() {
        isEditingText = false
        let cleanText = textValue.replacingOccurrences(of: ",", with: ".")
        if let parsed = Double(cleanText) {
            let clamped = min(max(parsed, range.lowerBound), range.upperBound)
            let rounded = (clamped / step).rounded() * step
            value = rounded
            textValue = String(format: format, rounded)
        } else {
            textValue = String(format: format, value)
        }
    }
}

// MARK: - Settings Scope Support
enum SettingScope {
    case global
    case online
    case lobbyHost
    
    var title: String {
        switch self {
        case .global: return L10n.settings.scopeGlobal
        case .online: return L10n.settings.scopeOnline
        case .lobbyHost: return L10n.settings.scopeLobbyHost
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
