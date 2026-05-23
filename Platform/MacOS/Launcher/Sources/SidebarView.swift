import SwiftUI
import AppKit

struct SidebarView: View {
    @ObservedObject var viewModel: LauncherViewModel

    private let neonBlue = Color(red: 0.1, green: 0.5, blue: 1.0)
    private let neonGreen = Color(red: 0.1, green: 0.9, blue: 0.4)

    var body: some View {
        VStack(spacing: 0) {
            // Title
            HStack {
                Image(systemName: "slider.horizontal.3")
                    .foregroundColor(neonBlue)
                Text(L10n.settings.title)
                    .font(.system(size: 16, weight: .black, design: .monospaced))
                    .foregroundColor(.white)
                Spacer()
            }
            .padding(.bottom, 16)
            
            // Scrolling Settings List
            ScrollView(.vertical, showsIndicators: false) {
                VStack(spacing: 14) {
                    _buildSidebarSettingToggle(
                        title: L10n.settings.windowedEdgeScroll,
                        description: L10n.settings.windowedEdgeScrollDesc,
                        isOn: $viewModel.isWindowedEdgeScrollEnabled
                    )
                }
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

    private func _buildSidebarSettingToggle(title: String, description: String, isOn: Binding<Bool>) -> some View {
        Button(action: { isOn.wrappedValue.toggle() }) {
            HStack(alignment: .top, spacing: 12) {
                Image(systemName: isOn.wrappedValue ? "checkmark.square.fill" : "square")
                    .font(.system(size: 18))
                    .foregroundColor(isOn.wrappedValue ? neonGreen : .white.opacity(0.3))
                    .padding(.top, 2)
                
                VStack(alignment: .leading, spacing: 4) {
                    Text(title)
                        .font(.system(size: 13, weight: .bold, design: .monospaced))
                        .foregroundColor(.white)
                        .multilineTextAlignment(.leading)
                    
                    Text(description)
                        .font(.system(size: 10, design: .monospaced))
                        .foregroundColor(.white.opacity(0.5))
                        .multilineTextAlignment(.leading)
                        .fixedSize(horizontal: false, vertical: true)
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
