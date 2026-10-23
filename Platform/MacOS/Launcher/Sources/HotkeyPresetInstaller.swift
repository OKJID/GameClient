import Foundation

final class HotkeyPresetInstaller: ObservableObject {
    @Published private(set) var installedRevision = 0

    private var isRunning = false

    func syncIfNeeded(_ package: HotkeyPresetPackage?, targets: [PatchTarget]) {
        guard let package, !isRunning, !targets.isEmpty else { return }

        let isCurrent = targets.allSatisfy { target in
            HotkeyPresetStore.isShippedCurrent(in: target.profile.hotkeyPresetsDirURL, packageVersion: package.packageVersion)
        }
        guard !isCurrent else { return }

        let workDir = FileManager.default.temporaryDirectory
            .appendingPathComponent("go_hotkey_presets_\(UUID().uuidString)", isDirectory: true)
        guard (try? FileManager.default.createDirectory(at: workDir, withIntermediateDirectories: true)) != nil else {
            return
        }

        isRunning = true
        downloadPart(0, package: package, targets: targets, workDir: workDir)
    }

    private func downloadPart(_ index: Int, package: HotkeyPresetPackage, targets: [PatchTarget], workDir: URL) {
        guard index < package.downloadURLs.count else {
            place(targets: targets, workDir: workDir)
            return
        }

        URLSession.shared.downloadTask(with: package.downloadURLs[index]) { [weak self] localURL, response, error in
            let zip = workDir.appendingPathComponent("part\(index + 1).zip")
            let statusCode = (response as? HTTPURLResponse)?.statusCode
            guard let self else { return }
            guard error == nil, statusCode == 200, let localURL,
                  (try? FileManager.default.moveItem(at: localURL, to: zip)) != nil else {
                self.finish(workDir: workDir, isInstalled: false)
                return
            }

            self.unpackPart(index, zip: zip, package: package, targets: targets, workDir: workDir)
        }.resume()
    }

    private func unpackPart(_ index: Int, zip: URL, package: HotkeyPresetPackage, targets: [PatchTarget], workDir: URL) {
        let process = Process()
        process.executableURL = URL(fileURLWithPath: "/usr/bin/unzip")
        process.arguments = ["-qo", zip.path, "-d", Self.extractedDir(in: workDir).path]
        process.terminationHandler = { [weak self] proc in
            guard let self else { return }
            guard proc.terminationStatus == 0 else {
                self.finish(workDir: workDir, isInstalled: false)
                return
            }

            self.downloadPart(index + 1, package: package, targets: targets, workDir: workDir)
        }

        do {
            try process.run()
        } catch {
            finish(workDir: workDir, isInstalled: false)
        }
    }

    private func place(targets: [PatchTarget], workDir: URL) {
        let extracted = Self.extractedDir(in: workDir)
        let isInstalled = targets.allSatisfy { target in
            let source = extracted.appendingPathComponent(target.profile.patchAssetsDirName, isDirectory: true)
            let presets = HotkeyPresetStore.shippedFiles(in: source)
            guard !presets.isEmpty else { return false }

            return (try? HotkeyPresetStore.replaceShipped(in: target.profile.hotkeyPresetsDirURL, with: presets)) != nil
        }

        finish(workDir: workDir, isInstalled: isInstalled)
    }

    private func finish(workDir: URL, isInstalled: Bool) {
        try? FileManager.default.removeItem(at: workDir)

        DispatchQueue.main.async {
            self.isRunning = false
            guard isInstalled else { return }
            self.installedRevision += 1
        }
    }

    private static func extractedDir(in workDir: URL) -> URL {
        workDir.appendingPathComponent("extracted", isDirectory: true)
    }
}
