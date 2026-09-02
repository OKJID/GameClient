import Foundation

struct PatchManifest {
    static let fileName = "AllowedArchives.txt"
    static let quarantineDirName = "Disabled"
    static let archiveExtension = "big"

    static func allowedArchives(at directory: URL) -> Set<String>? {
        let url = directory.appendingPathComponent(fileName)
        guard let raw = try? String(contentsOf: url, encoding: .utf8) else {
            return nil
        }

        let names = raw
            .split(whereSeparator: \.isNewline)
            .map { $0.trimmingCharacters(in: .whitespaces).lowercased() }
            .filter { !$0.isEmpty && !$0.hasPrefix("#") }

        return names.isEmpty ? nil : Set(names)
    }

    static func foreignArchives(at directory: URL, allowed: Set<String>) -> [URL] {
        let fm = FileManager.default
        guard let walker = fm.enumerator(at: directory, includingPropertiesForKeys: nil) else {
            return []
        }

        var found: [URL] = []
        for case let item as URL in walker {
            if item.lastPathComponent == quarantineDirName {
                walker.skipDescendants()
                continue
            }

            guard item.pathExtension.lowercased() == archiveExtension else { continue }
            guard !allowed.contains(item.lastPathComponent.lowercased()) else { continue }

            found.append(item)
        }

        return found
    }

    static func foreignArchives(at directory: URL) -> [URL] {
        guard let allowed = allowedArchives(at: directory) else { return [] }
        return foreignArchives(at: directory, allowed: allowed)
    }

    static func quarantine(_ archives: [URL], in directory: URL, fm: FileManager, log: ((String) -> Void)?) {
        guard !archives.isEmpty else { return }

        let quarantineDir = directory.appendingPathComponent(quarantineDirName)
        guard (try? fm.createDirectory(at: quarantineDir, withIntermediateDirectories: true)) != nil else {
            log?("[✗] Could not create \(quarantineDirName), foreign archives left in place\n")
            return
        }

        for archive in archives {
            let destination = quarantineDir.appendingPathComponent(archive.lastPathComponent)
            try? fm.removeItem(at: destination)
            guard (try? fm.moveItem(at: archive, to: destination)) != nil else {
                log?("[✗] Could not quarantine \(archive.lastPathComponent)\n")
                continue
            }

            log?("[*] Quarantined foreign archive: \(archive.lastPathComponent)\n")
        }
    }
}
