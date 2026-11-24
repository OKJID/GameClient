import AppKit

protocol OwnModFolderStore {
    func load() -> URL?
    func save(_ folder: URL)
}

struct UserDefaultsOwnModFolderStore: OwnModFolderStore {
    private static let folderKey = "OwnModFolder"

    func load() -> URL? {
        guard let path = UserDefaults.standard.string(forKey: Self.folderKey), !path.isEmpty else {
            return nil
        }

        return URL(fileURLWithPath: path, isDirectory: true)
    }

    func save(_ folder: URL) {
        UserDefaults.standard.set(folder.path, forKey: Self.folderKey)
    }
}

protocol OwnModFolderPicker {
    func pickFolder(startingAt folder: URL?) -> URL?
}

struct PanelOwnModFolderPicker: OwnModFolderPicker {
    func pickFolder(startingAt folder: URL?) -> URL? {
        let panel = NSOpenPanel()
        panel.canChooseFiles = false
        panel.canChooseDirectories = true
        panel.allowsMultipleSelection = false
        panel.message = OwnModText.pickerMessage
        panel.directoryURL = folder

        guard panel.runModal() == .OK else {
            return nil
        }

        return panel.url
    }
}
