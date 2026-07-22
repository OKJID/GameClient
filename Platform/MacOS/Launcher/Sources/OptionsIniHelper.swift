import Foundation

struct OptionsIniHelper {
    static var optionsFilePath: URL {
        GameProfile.current.optionsFileURL
    }

    static func readValue(forKey key: String) -> String? {
        let path = optionsFilePath
        guard FileManager.default.fileExists(atPath: path.path),
              let content = try? String(contentsOf: path, encoding: .utf8) else {
            return nil
        }
        
        let lines = content.components(separatedBy: .newlines)
        for line in lines {
            let parts = line.components(separatedBy: "=")
            if parts.count >= 2 {
                let k = parts[0].trimmingCharacters(in: .whitespacesAndNewlines)
                if k.lowercased() == key.lowercased() {
                    return parts[1...].joined(separator: "=").trimmingCharacters(in: .whitespacesAndNewlines)
                }
            }
        }
        return nil
    }

    static func writeValue(value: String, forKey key: String) {
        writeValues([key: value])
    }

    static func writeValues(_ updates: [String: String]) {
        let path = optionsFilePath
        let dir = path.deletingLastPathComponent()
        
        if !FileManager.default.fileExists(atPath: dir.path) {
            try? FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true, attributes: nil)
        }
        
        var lines: [String] = []
        if FileManager.default.fileExists(atPath: path.path),
           let content = try? String(contentsOf: path, encoding: .utf8) {
            lines = content.components(separatedBy: "\n")
        }
        
        var remainingUpdates = updates
        
        for i in 0..<lines.count {
            let line = lines[i]
            if line.trimmingCharacters(in: .whitespacesAndNewlines).hasPrefix(";") {
                continue
            }
            let parts = line.components(separatedBy: "=")
            if parts.count >= 2 {
                let k = parts[0].trimmingCharacters(in: .whitespacesAndNewlines)
                if let match = remainingUpdates.keys.first(where: { $0.lowercased() == k.lowercased() }) {
                    let val = remainingUpdates[match]!
                    lines[i] = "\(parts[0])= \(val)"
                    remainingUpdates.removeValue(forKey: match)
                }
            }
        }
        
        for (key, val) in remainingUpdates {
            lines.append("\(key) = \(val)")
        }
        
        while let last = lines.last, last.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty {
            lines.removeLast()
        }
        lines.append("")
        
        let newContent = lines.joined(separator: "\n")
        try? newContent.write(to: path, atomically: true, encoding: .utf8)
    }
}
