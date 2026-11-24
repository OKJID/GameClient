import Foundation

enum OwnModIssueSeverity: Equatable {
    case blocking
    case warning
}

struct OwnModIssue: Equatable, Identifiable {
    let code: String
    let severity: OwnModIssueSeverity
    let message: String

    var id: String { code }
}

struct OwnModValidationContext {
    let installRoot: URL?
}

protocol OwnModValidator {
    func validate(_ spec: OwnModSpec, context: OwnModValidationContext) -> OwnModIssue?
}

struct OwnModValidation {
    let validators: [OwnModValidator]

    static let standard = OwnModValidation(validators: [
        OwnModFolderExistsValidator(),
        OwnModGibArchivesValidator()
    ])

    func issues(for spec: OwnModSpec, context: OwnModValidationContext) -> [OwnModIssue] {
        validators.compactMap { $0.validate(spec, context: context) }
    }
}

struct OwnModFolderExistsValidator: OwnModValidator {
    func validate(_ spec: OwnModSpec, context: OwnModValidationContext) -> OwnModIssue? {
        var isDirectory: ObjCBool = false
        let exists = FileManager.default.fileExists(atPath: spec.folder.path, isDirectory: &isDirectory)
        if exists && isDirectory.boolValue {
            return nil
        }

        return OwnModIssue(
            code: "folder_missing",
            severity: .blocking,
            message: String(format: OwnModText.folderMissing, spec.folder.path)
        )
    }
}

struct OwnModGibArchivesValidator: OwnModValidator {
    private static let gibExtension = "gib"
    private static let searchDepth = 2

    func validate(_ spec: OwnModSpec, context: OwnModValidationContext) -> OwnModIssue? {
        let count = Self.gibArchiveCount(in: spec.folder)
        guard count > 0 else {
            return nil
        }

        return OwnModIssue(
            code: "gib_archives",
            severity: .warning,
            message: String(format: OwnModText.gibArchivesIgnored, count)
        )
    }

    private static func gibArchiveCount(in folder: URL) -> Int {
        guard let enumerator = FileManager.default.enumerator(
            at: folder,
            includingPropertiesForKeys: nil,
            options: [.skipsHiddenFiles, .skipsPackageDescendants]
        ) else {
            return 0
        }

        var count = 0
        for case let url as URL in enumerator {
            if enumerator.level >= searchDepth {
                enumerator.skipDescendants()
            }

            if url.pathExtension.lowercased() == gibExtension {
                count += 1
            }
        }

        return count
    }
}
