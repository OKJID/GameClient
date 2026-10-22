import Foundation

struct HotkeyEngineLaunch {
    let executable: URL
    let arguments: [String]
    let environment: [String: String]
}

enum HotkeyEngineRunner {
    static func dump(_ launch: HotkeyEngineLaunch, files: HotkeyFiles, completion: @escaping (Bool) -> Void) {
        run(launch, files: files, extraArguments: ["-dumpHotkeys", files.dumpURL.path], completion: completion)
    }

    static func apply(_ launch: HotkeyEngineLaunch, files: HotkeyFiles, completion: @escaping (Bool) -> Void) {
        let arguments = ["-applyHotkeys", files.editURL.path, "-dumpHotkeys", files.dumpURL.path]
        run(launch, files: files, extraArguments: arguments, completion: completion)
    }

    private static func run(
        _ launch: HotkeyEngineLaunch,
        files: HotkeyFiles,
        extraArguments: [String],
        completion: @escaping (Bool) -> Void
    ) {
        let startedAt = Date().addingTimeInterval(-1)
        let process = Process()
        process.executableURL = launch.executable
        process.currentDirectoryURL = launch.executable.deletingLastPathComponent()
        process.arguments = launch.arguments + extraArguments
        process.environment = launch.environment
        process.terminationHandler = { _ in
            let isDumpFresh = (files.dumpModificationDate() ?? .distantPast) >= startedAt
            DispatchQueue.main.async { completion(isDumpFresh) }
        }

        do {
            try process.run()
        } catch {
            completion(false)
        }
    }
}
