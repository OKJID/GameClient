import Foundation

enum RosettaInstallResult {
    case installed
    case cancelled
    case failed(String)
}

enum RosettaInstaller {
    private static let runtimePath = "/usr/libexec/rosetta/oahd"
    private static let installCommand = "/usr/sbin/softwareupdate --install-rosetta --agree-to-license"
    private static let userCancelledMarker = "User canceled"

    static var isAppleSilicon: Bool {
        var value: Int32 = 0
        var size = MemoryLayout<Int32>.size

        guard sysctlbyname("hw.optional.arm64", &value, &size, nil, 0) == 0 else { return false }
        return value == 1
    }

    static var isInstalled: Bool {
        FileManager.default.fileExists(atPath: runtimePath)
    }

    static func isRequired(toRun binary: URL) -> Bool {
        guard isAppleSilicon else { return false }
        guard !isInstalled else { return false }

        return !MachOReader.architectures(of: binary).contains(.arm64)
    }

    static func isArchitectureFailure(_ error: Error) -> Bool {
        let failure = error as NSError
        return failure.domain == NSPOSIXErrorDomain && failure.code == Int(EBADARCH)
    }

    static func install(completion: @escaping (RosettaInstallResult) -> Void) {
        DispatchQueue.global(qos: .userInitiated).async {
            completion(runInstaller())
        }
    }

    private static func runInstaller() -> RosettaInstallResult {
        let task = Process()
        task.executableURL = URL(fileURLWithPath: "/usr/bin/osascript")
        task.arguments = ["-e", "do shell script \"\(installCommand)\" with administrator privileges"]

        let errorPipe = Pipe()
        task.standardOutput = Pipe()
        task.standardError = errorPipe

        do {
            try task.run()
        } catch {
            return .failed(error.localizedDescription)
        }

        let errorData = errorPipe.fileHandleForReading.readDataToEndOfFile()
        task.waitUntilExit()

        let errorText = String(data: errorData, encoding: .utf8)?
            .trimmingCharacters(in: .whitespacesAndNewlines) ?? ""

        if errorText.contains(userCancelledMarker) {
            return .cancelled
        }

        if task.terminationStatus != 0 {
            return .failed(errorText.isEmpty ? "exit code \(task.terminationStatus)" : errorText)
        }

        guard isInstalled else {
            return .failed("Rosetta runtime is still missing after installation")
        }

        return .installed
    }
}

enum MachOArchitecture: UInt32 {
    case x86_64 = 0x0100_0007
    case arm64 = 0x0100_000C
}

enum MachOReader {
    private static let fatMagic: UInt32 = 0xCAFE_BABE
    private static let fat64Magic: UInt32 = 0xCAFE_BABF
    private static let thinMagic32: UInt32 = 0xFEED_FACE
    private static let thinMagic64: UInt32 = 0xFEED_FACF
    private static let fatEntrySize = 20
    private static let fat64EntrySize = 32
    private static let headerScanSize = 4096

    static func architectures(of binary: URL) -> Set<MachOArchitecture> {
        guard let header = try? Data(contentsOf: binary, options: .mappedIfSafe).prefix(headerScanSize) else {
            return []
        }

        guard let magic = header.integer(at: 0, bigEndian: true) else { return [] }

        if magic == fatMagic {
            return fatArchitectures(in: header, entrySize: fatEntrySize)
        }

        if magic == fat64Magic {
            return fatArchitectures(in: header, entrySize: fat64EntrySize)
        }

        return thinArchitecture(in: header, magic: magic)
    }

    private static func fatArchitectures(in header: Data, entrySize: Int) -> Set<MachOArchitecture> {
        guard let count = header.integer(at: 4, bigEndian: true) else { return [] }

        let entries = (0..<Int(count)).compactMap { index -> MachOArchitecture? in
            guard let cpuType = header.integer(at: 8 + index * entrySize, bigEndian: true) else { return nil }
            return MachOArchitecture(rawValue: cpuType)
        }

        return Set(entries)
    }

    private static func thinArchitecture(in header: Data, magic: UInt32) -> Set<MachOArchitecture> {
        let isLittleEndian = magic.byteSwapped == thinMagic32 || magic.byteSwapped == thinMagic64
        guard isLittleEndian || magic == thinMagic32 || magic == thinMagic64 else { return [] }

        guard let cpuType = header.integer(at: 4, bigEndian: !isLittleEndian),
              let architecture = MachOArchitecture(rawValue: cpuType) else {
            return []
        }

        return [architecture]
    }
}

private extension Data {
    func integer(at offset: Int, bigEndian: Bool) -> UInt32? {
        let start = startIndex + offset
        let end = start + MemoryLayout<UInt32>.size

        guard end <= endIndex else { return nil }

        let value = self[start..<end].reduce(UInt32(0)) { accumulated, byte in
            (accumulated << 8) | UInt32(byte)
        }

        return bigEndian ? value : value.byteSwapped
    }
}
