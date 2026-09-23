import SwiftUI
import AppKit

struct SettingsPageView: View {
    @ObservedObject var viewModel: LauncherViewModel

    private let minColumnWidth: CGFloat = 320
    private let minColumns = 2
    private let maxColumns = 4
    private let gridSpacing: CGFloat = 16
    private let pageInset: CGFloat = 32
    private var profile: GameProfile { viewModel.selectedProfile }
    private var accent: Color { profile.theme.accent }

    private func columnCount(for width: CGFloat) -> Int {
        let fitting = Int((width + gridSpacing) / (minColumnWidth + gridSpacing))
        return min(max(fitting, minColumns), maxColumns)
    }

    private func topics(in scope: SettingScope) -> [SettingTopic] {
        SettingTopic.allCases.filter { !profile.supportedKeys(scope: scope, topic: $0).isEmpty }
    }

    var body: some View {
        VStack(spacing: 0) {
            _buildHeader()
                .padding(.horizontal, pageInset)
                .padding(.top, 20)
                .padding(.bottom, 16)

            GeometryReader { geometry in
                ScrollView(.vertical, showsIndicators: true) {
                    VStack(alignment: .leading, spacing: 32) {
                        ForEach(profile.supportedScopes, id: \.self) { scope in
                            _buildScopeSection(scope, columns: columnCount(for: geometry.size.width - pageInset * 2))
                        }
                    }
                    .padding(.horizontal, pageInset)
                    .padding(.bottom, 32)
                }
            }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Color.black.opacity(0.6))
    }

    private func _buildHeader() -> some View {
        HStack(spacing: 16) {
            Button(action: { viewModel.route = .home }) {
                HStack(spacing: 6) {
                    Image(systemName: "chevron.left")
                        .font(.system(size: 11, weight: .bold))
                    Text(L10n.settings.back.uppercased())
                        .font(.system(size: 11, weight: .black, design: .monospaced))
                }
                .foregroundColor(accent)
                .padding(.horizontal, 12)
                .padding(.vertical, 7)
                .overlay(
                    RoundedRectangle(cornerRadius: 6)
                        .stroke(accent.opacity(0.45), lineWidth: 1)
                )
                .contentShape(RoundedRectangle(cornerRadius: 6))
            }
            .buttonStyle(PlainButtonStyle())
            .keyboardShortcut(.cancelAction)
            .onHover { inside in
                if inside { NSCursor.pointingHand.push() } else { NSCursor.pop() }
            }

            Image(systemName: "slider.horizontal.3")
                .foregroundColor(accent)
            Text(L10n.settings.title)
                .font(.system(size: 18, weight: .black, design: .monospaced))
                .foregroundColor(.white)
            Text(profile.displayName.uppercased())
                .font(.system(size: 11, weight: .bold, design: .monospaced))
                .foregroundColor(.white.opacity(0.45))

            Spacer()

            Button(action: { viewModel.resetAllSettings() }) {
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
    }

    private func _buildScopeSection(_ scope: SettingScope, columns: Int) -> some View {
        VStack(alignment: .leading, spacing: 14) {
            _buildScopeHeader(scope)

            LazyVGrid(
                columns: Array(repeating: GridItem(.flexible(), spacing: gridSpacing, alignment: .top), count: columns),
                alignment: .leading,
                spacing: gridSpacing
            ) {
                ForEach(topics(in: scope), id: \.self) { topic in
                    _buildTopicCard(topic, scope: scope)
                }
            }
        }
    }

    private func _buildScopeHeader(_ scope: SettingScope) -> some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack(spacing: 10) {
                ScopeChip(scope: scope)

                VStack {
                    Divider()
                        .background(scope.color.opacity(0.3))
                }
            }

            Text(scope.description)
                .font(.system(size: 11, design: .monospaced))
                .foregroundColor(.white.opacity(0.6))
                .fixedSize(horizontal: false, vertical: true)
        }
    }

    private func _buildTopicCard(_ topic: SettingTopic, scope: SettingScope) -> some View {
        VStack(alignment: .leading, spacing: 12) {
            Text(topic.title.uppercased())
                .font(.system(size: 10, weight: .black, design: .monospaced))
                .foregroundColor(accent.opacity(0.8))

            ForEach(profile.supportedKeys(scope: scope, topic: topic), id: \.self) { key in
                SettingRow(key: key, viewModel: viewModel, showsScope: false)
            }
        }
        .padding(14)
        .frame(maxWidth: .infinity, alignment: .topLeading)
        .background(Color.black.opacity(0.35))
        .clipShape(RoundedRectangle(cornerRadius: 8))
        .overlay(
            RoundedRectangle(cornerRadius: 8)
                .stroke(scope.color.opacity(0.2), lineWidth: 1)
        )
    }
}
