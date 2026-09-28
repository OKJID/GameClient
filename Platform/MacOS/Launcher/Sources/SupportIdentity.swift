import CryptoKit
import Foundation
import IOKit

enum SupportIdentity {
    static let environmentKey = "GENERALS_SUPPORT_ID"

    static let current: String = loadOrCreate()

    private struct Record: Codable {
        let supportId: String
        let device: String
    }

    private static let alphabet = Array("23456789ABCDEFGHJKMNPQRSTUVWXYZ")
    private static let groupLength = 4
    private static let fingerprintSalt = "GeneralsOnlineSupport:"

    private static var fileURL: URL {
        GameProfile.zeroHour.userDataDirURL.appendingPathComponent("GeneralsOnlineData/SupportID.json")
    }

    private static func loadOrCreate() -> String {
        let device = deviceFingerprint()
        if let record = readRecord(), record.device == device {
            return record.supportId
        }

        let record = Record(supportId: generateSupportId(), device: device)
        write(record)
        return record.supportId
    }

    private static func readRecord() -> Record? {
        guard let data = try? Data(contentsOf: fileURL) else { return nil }
        return try? JSONDecoder().decode(Record.self, from: data)
    }

    private static func write(_ record: Record) {
        let encoder = JSONEncoder()
        encoder.outputFormatting = [.prettyPrinted, .sortedKeys]
        guard let data = try? encoder.encode(record) else { return }

        try? FileManager.default.createDirectory(at: fileURL.deletingLastPathComponent(), withIntermediateDirectories: true)
        try? data.write(to: fileURL, options: .atomic)
    }

    private static func generateSupportId() -> String {
        let group = { String((0..<groupLength).map { _ in alphabet.randomElement()! }) }
        return "GO-\(group())-\(group())"
    }

    private static func deviceFingerprint() -> String {
        let hardwareId = platformUUID() ?? "unknown"
        let digest = SHA256.hash(data: Data((fingerprintSalt + hardwareId).utf8))
        return digest.prefix(8).map { String(format: "%02x", $0) }.joined()
    }

    private static func platformUUID() -> String? {
        let service = IOServiceGetMatchingService(kIOMainPortDefault, IOServiceMatching("IOPlatformExpertDevice"))
        guard service != 0 else { return nil }
        defer { IOObjectRelease(service) }

        let value = IORegistryEntryCreateCFProperty(service, kIOPlatformUUIDKey as CFString, kCFAllocatorDefault, 0)
        return value?.takeRetainedValue() as? String
    }
}
