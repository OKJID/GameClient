import SwiftUI
import AppKit

enum SettingTopic: CaseIterable {
    case interface
    case camera
    case performance
    case network
    case diagnostics

    var title: String {
        switch self {
        case .interface: return L10n.settings.interfaceSection
        case .camera: return L10n.settings.cameraSection
        case .performance: return L10n.settings.fpsSection
        case .network: return L10n.settings.networkSection
        case .diagnostics: return L10n.settings.diagnosticsSection
        }
    }
}

extension SettingKey {
    var scope: SettingScope {
        switch self {
        case .cameraMaxHeight: return .lobbyHost
        case .cameraMaxHeightLocal: return .offline
        case .altEndpoint: return .online
        default: return .global
        }
    }

    var topic: SettingTopic {
        switch self {
        case .gameLanguage, .windowedEdgeScroll, .showHotkeyLabels, .wasdMapScroll: return .interface
        case .cameraMaxHeight, .cameraMaxHeightLocal, .cameraMinHeight, .cameraSpeed: return .camera
        case .limitFramerate, .fpsLimit, .statsOverlay: return .performance
        case .altEndpoint: return .network
        case .verboseLogging: return .diagnostics
        }
    }
}

extension GameProfile {
    var supportedScopes: [SettingScope] {
        SettingScope.allCases.filter { scope in
            SettingKey.allCases.contains { $0.scope == scope && supports($0) }
        }
    }

    func supportedKeys(topic: SettingTopic) -> [SettingKey] {
        SettingKey.allCases.filter { $0.topic == topic && supports($0) }
    }

    func supportedKeys(scope: SettingScope, topic: SettingTopic) -> [SettingKey] {
        supportedKeys(topic: topic).filter { $0.scope == scope }
    }
}

struct SettingRow: View {
    let key: SettingKey
    @ObservedObject var viewModel: LauncherViewModel
    var showsScope: Bool = true

    private var accent: Color { viewModel.selectedProfile.theme.accent }

    var body: some View {
        _buildControl()
    }

    @ViewBuilder
    private func _buildControl() -> some View {
        switch key {
        case .gameLanguage:
            GameLanguagePickerCard(viewModel: viewModel, showsScope: showsScope)
        case .windowedEdgeScroll:
            _buildToggle(L10n.settings.windowedEdgeScroll, L10n.settings.windowedEdgeScrollDesc, $viewModel.isWindowedEdgeScrollEnabled)
        case .showHotkeyLabels:
            _buildToggle(L10n.settings.showHotkeyLabels, L10n.settings.showHotkeyLabelsDesc, $viewModel.showHotkeyLabels)
        case .wasdMapScroll:
            _buildToggle(L10n.settings.wasdMapScroll, L10n.settings.wasdMapScrollDesc, $viewModel.wasdMapScroll)
        case .cameraMaxHeight:
            _buildSlider(
                L10n.settings.cameraMaxHeight, $viewModel.cameraMaxHeight,
                SettingsDefaults.cameraMaxHeightRange, SettingsDefaults.cameraMaxHeightStep,
                SettingsDefaults.cameraMaxHeightFormat, SettingsDefaults.cameraMaxHeight
            )
        case .cameraMaxHeightLocal:
            _buildSlider(
                L10n.settings.cameraMaxHeightLocal, $viewModel.cameraMaxHeightLocal,
                SettingsDefaults.cameraMaxHeightLocalRange, SettingsDefaults.cameraMaxHeightLocalStep,
                SettingsDefaults.cameraMaxHeightLocalFormat, SettingsDefaults.cameraMaxHeightLocal
            )
        case .cameraMinHeight:
            _buildSlider(
                L10n.settings.cameraMinHeight, $viewModel.cameraMinHeight,
                SettingsDefaults.cameraMinHeightRange, SettingsDefaults.cameraMinHeightStep,
                SettingsDefaults.cameraMinHeightFormat, SettingsDefaults.cameraMinHeight
            )
        case .cameraSpeed:
            _buildSlider(
                L10n.settings.cameraSpeed, $viewModel.cameraMoveSpeed,
                SettingsDefaults.cameraMoveSpeedRange, SettingsDefaults.cameraMoveSpeedStep,
                SettingsDefaults.cameraMoveSpeedFormat, SettingsDefaults.cameraMoveSpeed
            )
        case .limitFramerate:
            _buildToggle(L10n.settings.limitFps, "", $viewModel.limitFramerate)
        case .fpsLimit:
            _buildSlider(
                "", $viewModel.fpsLimit,
                SettingsDefaults.fpsLimitRange, SettingsDefaults.fpsLimitStep,
                SettingsDefaults.fpsLimitFormat, SettingsDefaults.fpsLimit,
                isDisabled: !viewModel.limitFramerate
            )
        case .statsOverlay:
            _buildToggle(L10n.settings.statsOverlay, "", $viewModel.statsOverlay)
        case .altEndpoint:
            _buildToggle(L10n.settings.altEndpoint, "", $viewModel.useAlternativeEndpoint)
        case .verboseLogging:
            VStack(spacing: 16) {
                _buildToggle(L10n.settings.verboseLogging, "", $viewModel.verboseLogging)
                ShareLogsCard(accent: accent)
            }
        }
    }

    private func _buildToggle(_ title: String, _ description: String, _ isOn: Binding<Bool>) -> some View {
        SettingToggleCard(
            title: title,
            description: description,
            isOn: isOn,
            scope: key.scope,
            accent: accent,
            showsScope: showsScope
        )
    }

    private func _buildSlider(
        _ title: String,
        _ value: Binding<Double>,
        _ range: ClosedRange<Double>,
        _ step: Double,
        _ format: String,
        _ defaultValue: Double,
        isDisabled: Bool = false
    ) -> some View {
        SettingsSliderField(
            title: title,
            value: value,
            range: range,
            step: step,
            format: format,
            defaultValue: defaultValue,
            isDisabled: isDisabled,
            scope: key.scope,
            showsScope: showsScope
        )
    }
}

struct SettingRowOffsetsKey: PreferenceKey {
    static var defaultValue: [SettingKey: CGFloat] = [:]

    static func reduce(value: inout [SettingKey: CGFloat], nextValue: () -> [SettingKey: CGFloat]) {
        value.merge(nextValue()) { _, next in next }
    }
}

struct SettingsSectionHeader: View {
    let title: String
    let accent: Color

    var body: some View {
        HStack(spacing: 8) {
            Text(title.uppercased())
                .font(.system(size: 10, weight: .black, design: .monospaced))
                .foregroundColor(accent.opacity(0.8))

            VStack {
                Divider()
                    .background(accent.opacity(0.15))
            }
        }
        .padding(.top, 8)
        .padding(.bottom, 4)
    }
}

struct SettingToggleCard: View {
    let title: String
    let description: String
    @Binding var isOn: Bool
    let scope: SettingScope
    let accent: Color
    var showsScope: Bool = true

    private let neonGreen = Color(red: 0.1, green: 0.9, blue: 0.4)

    var body: some View {
        Button(action: { isOn.toggle() }) {
            HStack(alignment: .top, spacing: 12) {
                Image(systemName: isOn ? "checkmark.square.fill" : "square")
                    .font(.system(size: 18))
                    .foregroundColor(isOn ? neonGreen : .white.opacity(0.3))
                    .padding(.top, 2)

                VStack(alignment: .leading, spacing: 4) {
                    HStack(alignment: .center, spacing: 6) {
                        Text(title)
                            .font(.system(size: 13, weight: .bold, design: .monospaced))
                            .foregroundColor(.white)
                            .multilineTextAlignment(.leading)

                        if showsScope {
                            ScopeChip(scope: scope)
                        }
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
            .background(Color.white.opacity(isOn ? 0.05 : 0.02))
            .clipShape(RoundedRectangle(cornerRadius: 6))
            .overlay(
                RoundedRectangle(cornerRadius: 6)
                    .stroke(isOn ? accent.opacity(0.4) : Color.white.opacity(0.05), lineWidth: 1)
            )
        }
        .buttonStyle(PlainButtonStyle())
        .onHover { inside in
            if inside { NSCursor.pointingHand.push() } else { NSCursor.pop() }
        }
    }
}

struct GameLanguagePickerCard: View {
    @ObservedObject var viewModel: LauncherViewModel
    var showsScope: Bool = true

    private var profile: GameProfile { viewModel.selectedProfile }

    private var options: [String] {
        let installed = viewModel.installDirectory(for: profile)
            .map { profile.installedLanguages(at: $0) } ?? GameProfile.fallbackLanguages
        return installed.contains(viewModel.gameLanguage) ? installed : installed + [viewModel.gameLanguage]
    }

    var body: some View {
        VStack(alignment: .leading, spacing: 6) {
            HStack(spacing: 8) {
                Text(L10n.settings.gameLanguage)
                    .font(.system(size: 12, weight: .medium, design: .monospaced))
                    .foregroundColor(.white.opacity(0.9))

                if showsScope {
                    ScopeChip(scope: .global)
                }
            }

            Picker("", selection: $viewModel.gameLanguage) {
                ForEach(options, id: \.self) { lang in
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
}

struct ShareLogsCard: View {
    let accent: Color

    var body: some View {
        Button(action: { LogsSharer.share() }) {
            HStack(alignment: .top, spacing: 12) {
                Image(systemName: "square.and.arrow.up")
                    .font(.system(size: 16))
                    .foregroundColor(accent)
                    .padding(.top, 2)

                VStack(alignment: .leading, spacing: 4) {
                    Text(L10n.settings.shareLogs)
                        .font(.system(size: 13, weight: .bold, design: .monospaced))
                        .foregroundColor(.white)

                    Text(L10n.settings.shareLogsHint)
                        .font(.system(size: 10, design: .monospaced))
                        .foregroundColor(.white.opacity(0.5))
                        .multilineTextAlignment(.leading)
                        .fixedSize(horizontal: false, vertical: true)
                }
                Spacer()
            }
            .padding(12)
            .background(Color.white.opacity(0.02))
            .clipShape(RoundedRectangle(cornerRadius: 6))
            .overlay(
                RoundedRectangle(cornerRadius: 6)
                    .stroke(accent.opacity(0.3), lineWidth: 1)
            )
        }
        .buttonStyle(PlainButtonStyle())
        .onHover { inside in
            if inside { NSCursor.pointingHand.push() } else { NSCursor.pop() }
        }
    }
}
