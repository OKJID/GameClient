import Foundation

struct HotkeyEngineLaunch {
    let executable: URL
    let arguments: [String]
    let environment: [String: String]
}

enum HotkeyEngineRunner {
    static func dump(_ launch: HotkeyEngineLaunch, files: HotkeyFiles, completion: @escaping (Bool) -> Void) {
        run(launch, extraArguments: ["-dumpHotkeys", files.dumpURL.path], output: files.dumpURL, completion: completion)
    }

    static func apply(_ launch: HotkeyEngineLaunch, files: HotkeyFiles, completion: @escaping (Bool) -> Void) {
        let arguments = ["-applyHotkeys", files.editURL.path, "-dumpHotkeys", files.dumpURL.path]
        run(launch, extraArguments: arguments, output: files.dumpURL, completion: completion)
    }

    static func importPreset(
        _ launch: HotkeyEngineLaunch,
        from source: URL,
        to preset: URL,
        completion: @escaping (Bool) -> Void
    ) {
        let arguments = ["-importHotkeys", source.path, "-importHotkeysTo", preset.path]
        run(launch, extraArguments: arguments, output: preset, completion: completion)
    }

    static func export(_ launch: HotkeyEngineLaunch, to destination: URL, completion: @escaping (Bool) -> Void) {
        run(launch, extraArguments: ["-exportHotkeys", destination.path], output: destination, completion: completion)
    }

    private static func run(
        _ launch: HotkeyEngineLaunch,
        extraArguments: [String],
        output: URL,
        completion: @escaping (Bool) -> Void
    ) {
        try? FileManager.default.createDirectory(at: output.deletingLastPathComponent(), withIntermediateDirectories: true)
        let startedAt = Date().addingTimeInterval(-1)
        let process = Process()
        process.executableURL = launch.executable
        process.currentDirectoryURL = launch.executable.deletingLastPathComponent()
        process.arguments = launch.arguments + extraArguments
        process.environment = launch.environment
        process.terminationHandler = { _ in
            let isOutputFresh = (modificationDate(of: output) ?? .distantPast) >= startedAt
            DispatchQueue.main.async { completion(isOutputFresh) }
        }

        do {
            try process.run()
        } catch {
            completion(false)
        }
    }

    private static func modificationDate(of url: URL) -> Date? {
        let attributes = try? FileManager.default.attributesOfItem(atPath: url.path)
        return attributes?[.modificationDate] as? Date
    }
}
