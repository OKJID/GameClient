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
        OwnModFolderExistsValidator()
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
