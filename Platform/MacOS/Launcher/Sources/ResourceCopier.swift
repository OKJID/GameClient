import Foundation

enum ResourceCopyState: Equatable {
    case idle
    case copying(progress: Double)
    case completed
    case failed(String)

    var isRunning: Bool {
        guard case .copying = self else { return false }
        return true
    }

    var progressText: String {
        guard case .copying(let progress) = self else { return "" }
        return String(format: L10n.local.copying, progress * 100)
    }
}

enum ResourceCopyError: LocalizedError {
    case unreadableSource(URL)
    case unwritableDestination(URL)
    case destinationNotEmpty(URL)

    var errorDescription: String? {
        switch self {
        case .unreadableSource(let url): return "No read access to \(url.path)"
        case .unwritableDestination(let url): return "No write access to \(url.path)"
        case .destinationNotEmpty(let url): return "\(url.path) already exists and is not empty"
        }
    }
}

class ResourceCopier: ObservableObject {
    private struct CopyItem {
        let source: URL
        let target: URL
        let bytes: Int64
    }

    @Published private(set) var state: ResourceCopyState = .idle

    static let destinationDirName = "Generals Online Mac-Native"
    private static let stagingSuffix = ".copying"

    static var destinationRoot: URL {
        FileManager.default.homeDirectoryForCurrentUser
            .appendingPathComponent(destinationDirName, isDirectory: true)
    }

    func copy(_ sources: [URL], to destination: URL, onCompleted: @escaping (URL) -> Void) {
        guard !state.isRunning, !sources.isEmpty else { return }

        state = .copying(progress: 0)

        DispatchQueue.global(qos: .userInitiated).async {
            let result = Result { try self.copyAll(sources, to: destination) }
            DispatchQueue.main.async {
                self.finish(result, destination: destination, onCompleted: onCompleted)
            }
        }
    }

    func reset() {
        guard !state.isRunning else { return }
        state = .idle
    }

    private func finish(_ result: Result<Void, Error>, destination: URL, onCompleted: (URL) -> Void) {
        switch result {
        case .success:
            state = .completed
            onCompleted(destination)
        case .failure(let error):
            state = .failed(error.localizedDescription)
        }
    }

    private func copyAll(_ sources: [URL], to destination: URL) throws {
        let fm = FileManager.default
        try Self.checkAccess(sources, destination: destination, fm: fm)

        let createsRoot = !fm.fileExists(atPath: destination.path)
        do {
            try fm.createDirectory(at: destination, withIntermediateDirectories: true)
            try Self.prepareStaging(for: sources, in: destination, fm: fm)
            try copyContents(of: sources, to: destination, fm: fm)
            try Self.grantOwnerWrite(to: sources, in: destination, fm: fm)
            try Self.promoteStaging(sources, in: destination, fm: fm)
        } catch {
            Self.rollBack(sources, in: destination, removesRoot: createsRoot, fm: fm)
            throw error
        }
    }

    private func copyContents(of sources: [URL], to destination: URL, fm: FileManager) throws {
        let items = try sources.flatMap { try Self.copyItems(of: $0, in: destination, fm: fm) }
        let total = max(items.reduce(Int64(0)) { $0 + $1.bytes }, 1)
        var copied: Int64 = 0

        for item in items {
            try fm.copyItem(at: item.source, to: item.target)
            copied += item.bytes
            publishProgress(Double(copied) / Double(total))
        }
    }

    private func publishProgress(_ fraction: Double) {
        DispatchQueue.main.async {
            guard self.state.isRunning else { return }
            self.state = .copying(progress: fraction)
        }
    }

    private static func checkAccess(_ sources: [URL], destination: URL, fm: FileManager) throws {
        if let unreadable = sources.first(where: { !fm.isReadableFile(atPath: $0.path) }) {
            throw ResourceCopyError.unreadableSource(unreadable)
        }

        let writableDir = fm.fileExists(atPath: destination.path) ? destination : destination.deletingLastPathComponent()
        guard fm.isWritableFile(atPath: writableDir.path) else {
            throw ResourceCopyError.unwritableDestination(writableDir)
        }

        let visibleEntries = (try? fm.contentsOfDirectory(at: destination, includingPropertiesForKeys: nil, options: .skipsHiddenFiles)) ?? []
        guard visibleEntries.isEmpty else {
            throw ResourceCopyError.destinationNotEmpty(destination)
        }
    }

    private static func prepareStaging(for sources: [URL], in destination: URL, fm: FileManager) throws {
        for source in sources {
            let staging = stagingURL(for: source, in: destination)
            try? fm.removeItem(at: staging)
            try fm.createDirectory(at: staging, withIntermediateDirectories: false)
        }
    }

    private static func copyItems(of source: URL, in destination: URL, fm: FileManager) throws -> [CopyItem] {
        let staging = stagingURL(for: source, in: destination)
        return try fm.contentsOfDirectory(at: source, includingPropertiesForKeys: nil).map { child in
            CopyItem(
                source: child,
                target: staging.appendingPathComponent(child.lastPathComponent),
                bytes: byteSize(of: child, fm: fm)
            )
        }
    }

    private static func grantOwnerWrite(to sources: [URL], in destination: URL, fm: FileManager) throws {
        for source in sources {
            let staging = stagingURL(for: source, in: destination)
            try addOwnerWrite(to: staging, fm: fm)

            guard let enumerator = fm.enumerator(at: staging, includingPropertiesForKeys: nil) else { continue }
            for case let item as URL in enumerator {
                try addOwnerWrite(to: item, fm: fm)
            }
        }
    }

    private static func addOwnerWrite(to url: URL, fm: FileManager) throws {
        let attributes = try fm.attributesOfItem(atPath: url.path)
        guard attributes[.type] as? FileAttributeType != .typeSymbolicLink else { return }

        let permissions = (attributes[.posixPermissions] as? NSNumber)?.intValue ?? 0
        guard permissions & 0o200 == 0 else { return }

        try fm.setAttributes([.posixPermissions: permissions | 0o200], ofItemAtPath: url.path)
    }

    private static func promoteStaging(_ sources: [URL], in destination: URL, fm: FileManager) throws {
        for source in sources {
            try fm.moveItem(at: stagingURL(for: source, in: destination), to: finalURL(for: source, in: destination))
        }
    }

    private static func rollBack(_ sources: [URL], in destination: URL, removesRoot: Bool, fm: FileManager) {
        if removesRoot {
            try? fm.removeItem(at: destination)
            return
        }

        for source in sources {
            try? fm.removeItem(at: stagingURL(for: source, in: destination))
            try? fm.removeItem(at: finalURL(for: source, in: destination))
        }
    }

    private static func stagingURL(for source: URL, in destination: URL) -> URL {
        destination.appendingPathComponent(".\(source.lastPathComponent)\(stagingSuffix)", isDirectory: true)
    }

    private static func finalURL(for source: URL, in destination: URL) -> URL {
        destination.appendingPathComponent(source.lastPathComponent, isDirectory: true)
    }

    private static func byteSize(of url: URL, fm: FileManager) -> Int64 {
        var total = fileSize(of: url)
        guard let enumerator = fm.enumerator(at: url, includingPropertiesForKeys: [.fileSizeKey]) else {
            return total
        }

        for case let child as URL in enumerator {
            total += fileSize(of: child)
        }
        return total
    }

    private static func fileSize(of url: URL) -> Int64 {
        Int64((try? url.resourceValues(forKeys: [.fileSizeKey]))?.fileSize ?? 0)
    }
}
