import Foundation

enum ApiCache {
    struct DiskAccessError: Error {}

    private static let bundleSubdirectory = "api"

    static var folderURL: URL? {
        FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask).first?
            .appendingPathComponent("GeneralsOnlineLauncher", isDirectory: true)
    }

    static func load<Value>(_ name: String, decode: (Data) -> Value?) throws -> Value? {
        if let cached = read(name).flatMap(decode) {
            return cached
        }

        try restoreFromBundle(name)
        return read(name).flatMap(decode)
    }

    static func write(_ data: Data, as name: String) throws {
        guard let folderURL else {
            throw DiskAccessError()
        }

        do {
            try FileManager.default.createDirectory(at: folderURL, withIntermediateDirectories: true)
            try data.write(to: folderURL.appendingPathComponent(name), options: .atomic)
        } catch {
            throw DiskAccessError()
        }
    }

    private static func read(_ name: String) -> Data? {
        guard let folderURL else { return nil }
        return try? Data(contentsOf: folderURL.appendingPathComponent(name))
    }

    private static func restoreFromBundle(_ name: String) throws {
        guard let bundled = Bundle.main.url(forResource: name, withExtension: nil, subdirectory: bundleSubdirectory),
              let data = try? Data(contentsOf: bundled) else {
            return
        }

        try write(data, as: name)
    }
}
