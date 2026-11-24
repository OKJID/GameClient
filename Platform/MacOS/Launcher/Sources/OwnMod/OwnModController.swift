import AppKit
import Combine

final class OwnModController: ObservableObject {
    private let store: OwnModFolderStore
    private let picker: OwnModFolderPicker
    private let validation: OwnModValidation
    private let artworkLocator: OwnModArtworkLocator
    private let contextProvider: () -> OwnModValidationContext

    init(
        store: OwnModFolderStore = UserDefaultsOwnModFolderStore(),
        picker: OwnModFolderPicker = PanelOwnModFolderPicker(),
        validation: OwnModValidation = .standard,
        artworkLocator: OwnModArtworkLocator = .standard,
        contextProvider: @escaping () -> OwnModValidationContext
    ) {
        self.store = store
        self.picker = picker
        self.validation = validation
        self.artworkLocator = artworkLocator
        self.contextProvider = contextProvider
        state = _resolveState(for: store.load())
        _loadArtwork(for: state.spec?.folder)
    }

    @Published private(set) var state: OwnModState = .empty
    @Published private(set) var artwork: NSImage?

    var banner: NSImage? {
        guard state.spec != nil else { return nil }
        return artwork ?? ModArtwork.defaultBanner
    }

    func chooseFolder() {
        guard let folder = picker.pickFolder(startingAt: state.spec?.folder) else {
            return
        }

        store.save(folder)
        _apply(_resolveState(for: folder))
    }

    func revalidate() {
        _apply(_resolveState(for: state.spec?.folder))
    }

    func launchArguments() -> Result<[String], OwnModLaunchFailure> {
        revalidate()

        guard let spec = state.spec else {
            return .failure(.folderNotChosen)
        }

        if let issue = state.blockingIssue {
            return .failure(.rejected(issue))
        }

        return .success(OwnModLaunchArguments.arguments(for: spec))
    }

    private func _apply(_ newState: OwnModState) {
        guard newState != state else { return }
        state = newState
        _loadArtwork(for: newState.spec?.folder)
    }

    private func _loadArtwork(for folder: URL?) {
        guard let folder else {
            artwork = nil
            return
        }

        let locator = artworkLocator
        DispatchQueue.global(qos: .userInitiated).async { [weak self] in
            let image = locator.artworkURL(in: folder).flatMap(NSImage.init(contentsOf:))

            DispatchQueue.main.async {
                guard let self, self.state.spec?.folder == folder else { return }
                self.artwork = image
            }
        }
    }

    private func _resolveState(for folder: URL?) -> OwnModState {
        guard let folder else {
            return .empty
        }

        let spec = OwnModSpec(folder: folder, baseGameID: OwnModProfile.baseGameID)
        return .selected(spec, issues: validation.issues(for: spec, context: contextProvider()))
    }
}
