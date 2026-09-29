import AppKit
import Combine
import SwiftUI

final class ModArtwork: ObservableObject {
    static let shared = ModArtwork()

    static let defaultBanner = bundledImage("mod_banner_default")
    static let defaultMedallion = bundledImage("medallion_logo")

    private static let pathPrefix = "mods/"
    private static let pathExtension = ".png"
    private static let pathDepth = 3
    private static let requestTimeout: TimeInterval = 15

    @Published private(set) var images: [String: NSImage] = [:]

    private var pending: Set<String> = []

    private init() {}

    static func validPath(_ path: String) -> String? {
        guard path.hasPrefix(pathPrefix), path.hasSuffix(pathExtension) else { return nil }

        let components = path.split(separator: "/")
        guard components.count == pathDepth, !components.contains("..") else { return nil }
        return path
    }

    func image(at path: String?) -> NSImage? {
        guard let path else { return nil }
        return images[path]
    }

    func request(_ path: String?) {
        guard let path, images[path] == nil, !pending.contains(path) else { return }
        pending.insert(path)

        if let cached = Self.cachedImage(at: path) {
            publish(cached, at: path)
            return
        }

        download(path)
    }

    func prune(keeping paths: Set<String>) {
        guard !paths.isEmpty, let root = Self.cacheURL(for: Self.pathPrefix) else { return }

        DispatchQueue.global(qos: .utility).async {
            Self.removeFiles(in: root, except: paths)
        }
    }

    private static func removeFiles(in root: URL, except paths: Set<String>) {
        let fm = FileManager.default
        guard let files = fm.enumerator(at: root, includingPropertiesForKeys: [.isRegularFileKey]) else { return }

        for case let url as URL in files {
            guard (try? url.resourceValues(forKeys: [.isRegularFileKey]).isRegularFile) == true else { continue }
            guard !paths.contains(relativePath(of: url)) else { continue }

            try? fm.removeItem(at: url)
        }
    }

    private static func relativePath(of url: URL) -> String {
        url.pathComponents.suffix(pathDepth).joined(separator: "/")
    }

    private func download(_ path: String) {
        let url = Submission.apiBase.appendingPathComponent(path)
        let request = URLRequest(url: url, timeoutInterval: Self.requestTimeout)

        URLSession.shared.dataTask(with: request) { [weak self] data, response, _ in
            let statusCode = (response as? HTTPURLResponse)?.statusCode
            let image = statusCode == 200 ? data.flatMap(NSImage.init(data:)) : nil

            DispatchQueue.main.async {
                guard let self else { return }
                guard let data, let image else {
                    self.pending.remove(path)
                    return
                }

                Self.store(data, at: path)
                self.publish(image, at: path)
            }
        }.resume()
    }

    private func publish(_ image: NSImage, at path: String) {
        DispatchQueue.main.async {
            self.pending.remove(path)
            self.images[path] = image
        }
    }

    private static func cacheURL(for path: String) -> URL? {
        ApiCache.folderURL?.appendingPathComponent(path)
    }

    private static func cachedImage(at path: String) -> NSImage? {
        guard let url = cacheURL(for: path) else { return nil }
        return NSImage(contentsOf: url)
    }

    private static func store(_ data: Data, at path: String) {
        guard let url = cacheURL(for: path) else { return }

        try? FileManager.default.createDirectory(at: url.deletingLastPathComponent(), withIntermediateDirectories: true)
        try? data.write(to: url, options: .atomic)
    }

    private static func bundledImage(_ name: String) -> NSImage? {
        guard let path = Bundle.main.path(forResource: name, ofType: "png") else { return nil }
        return NSImage(contentsOfFile: path)
    }
}

struct ModArtworkImage: View {
    let path: String?
    let fallback: NSImage?
    let contentMode: ContentMode

    @ObservedObject private var artwork = ModArtwork.shared

    private var image: NSImage? { artwork.image(at: path) ?? fallback }

    var body: some View {
        Group {
            if let image {
                Image(nsImage: image)
                    .resizable()
                    .aspectRatio(contentMode: contentMode)
            } else {
                Color.clear
            }
        }
        .onAppear { artwork.request(path) }
        .onChange(of: path) { artwork.request($0) }
    }
}
