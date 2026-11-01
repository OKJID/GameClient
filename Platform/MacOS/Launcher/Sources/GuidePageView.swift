import SwiftUI
import AppKit

struct GuidePageView: View {
    @ObservedObject var viewModel: LauncherViewModel
    @ObservedObject private var catalog = GuideCatalog.shared

    @State private var searchQuery = ""

    private let pageInset: CGFloat = 32
    private let contentsWidth: CGFloat = 280
    private let searchWidth: CGFloat = 260

    private var accent: Color { viewModel.selectedProfile.theme.accent }
    private var language: String { viewModel.selectedLanguage }

    private var currentArticle: GuideArticle? {
        viewModel.guideArticleID.flatMap(catalog.article(id:)) ?? catalog.articles.first
    }

    private var hasSearchResults: Bool {
        catalog.articles.contains { $0.matches(searchQuery, language: language) }
    }

    private func visibleArticles(in section: GuideSection) -> [GuideArticle] {
        catalog.articles(in: section).filter { $0.matches(searchQuery, language: language) }
    }

    private func reportShownArticle() {
        guard let id = currentArticle?.id else { return }
        Analytics.logGuideArticleShown(id: id)
    }

    var body: some View {
        VStack(spacing: 0) {
            _buildHeader()
                .padding(.horizontal, pageInset)
                .padding(.top, 20)
                .padding(.bottom, 16)

            _buildBody()
                .padding(.horizontal, pageInset)
                .padding(.bottom, 24)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Color.black.opacity(0.6))
        .task(id: currentArticle?.id) { reportShownArticle() }
    }

    private func _buildHeader() -> some View {
        HStack(spacing: 16) {
            PageBackButton(accent: accent) { viewModel.route = .home }

            Image(systemName: "book")
                .foregroundColor(accent)
            Text(L10n.guide.title)
                .font(.system(size: 18, weight: .black, design: .monospaced))
                .foregroundColor(.white)

            Spacer()

            _buildSearchField()
        }
    }

    private func _buildSearchField() -> some View {
        HStack(spacing: 6) {
            Image(systemName: "magnifyingglass")
                .font(.system(size: 11, weight: .bold))

            TextField(L10n.guide.search, text: $searchQuery)
                .textFieldStyle(PlainTextFieldStyle())
                .font(.system(size: 12, weight: .bold, design: .monospaced))
                .foregroundColor(.white)
                .onExitCommand { searchQuery = "" }

            if !searchQuery.isEmpty {
                Button(action: { searchQuery = "" }) {
                    Image(systemName: "xmark.circle.fill")
                        .font(.system(size: 11))
                }
                .buttonStyle(PlainButtonStyle())
            }
        }
        .foregroundColor(.white.opacity(0.5))
        .padding(.horizontal, 10)
        .frame(width: searchWidth, height: 30)
        .background(Color.black.opacity(0.45))
        .overlay(RoundedRectangle(cornerRadius: 6).stroke(accent.opacity(0.3), lineWidth: 1))
        .clipShape(RoundedRectangle(cornerRadius: 6))
    }

    @ViewBuilder
    private func _buildBody() -> some View {
        if let article = currentArticle {
            HStack(alignment: .top, spacing: 16) {
                _buildContents()
                    .frame(width: contentsWidth)

                _buildArticle(article)
            }
        } else {
            _buildNotice(L10n.guide.empty)
                .frame(maxHeight: .infinity, alignment: .top)
        }
    }

    private func _buildContents() -> some View {
        ScrollView(.vertical, showsIndicators: false) {
            VStack(alignment: .leading, spacing: 4) {
                ForEach(catalog.sections) { section in
                    _buildSection(section)
                }

                if !hasSearchResults {
                    _buildNotice(L10n.guide.noResults)
                }
            }
        }
    }

    @ViewBuilder
    private func _buildSection(_ section: GuideSection) -> some View {
        let articles = visibleArticles(in: section)
        if !articles.isEmpty {
            SettingsSectionHeader(title: section.title(for: language), accent: accent)

            ForEach(articles) { article in
                _buildContentsRow(article)
            }
        }
    }

    private func _buildContentsRow(_ article: GuideArticle) -> some View {
        let isSelected = article.id == currentArticle?.id

        return Button(action: { viewModel.guideArticleID = article.id }) {
            Text(article.text(for: language)?.title ?? article.id)
                .font(.system(size: 12, weight: isSelected ? .black : .medium, design: .monospaced))
                .foregroundColor(isSelected ? .white : .white.opacity(0.65))
                .multilineTextAlignment(.leading)
                .frame(maxWidth: .infinity, alignment: .leading)
                .padding(.horizontal, 10)
                .padding(.vertical, 7)
                .background(isSelected ? accent.opacity(0.18) : Color.clear)
                .clipShape(RoundedRectangle(cornerRadius: 6))
                .contentShape(Rectangle())
        }
        .buttonStyle(PlainButtonStyle())
        .onHover { inside in
            if inside { NSCursor.pointingHand.push() } else { NSCursor.pop() }
        }
    }

    private func _buildArticle(_ article: GuideArticle) -> some View {
        let text = article.text(for: language)

        return VStack(alignment: .leading, spacing: 12) {
            Text(text?.title ?? article.id)
                .font(.system(size: 16, weight: .black, design: .monospaced))
                .foregroundColor(.white)

            GuideArticleWebView(
                html: text?.body ?? "",
                accent: accent,
                articleID: article.id,
                onLink: viewModel.follow
            )
        }
        .padding(18)
        .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
        .background(Color.black.opacity(0.35))
        .clipShape(RoundedRectangle(cornerRadius: 8))
        .overlay(RoundedRectangle(cornerRadius: 8).stroke(accent.opacity(0.2), lineWidth: 1))
    }

    private func _buildNotice(_ text: String) -> some View {
        Text(text)
            .font(.system(size: 12, weight: .medium, design: .monospaced))
            .foregroundColor(.white.opacity(0.5))
            .padding(.vertical, 12)
            .frame(maxWidth: .infinity, alignment: .leading)
    }
}
