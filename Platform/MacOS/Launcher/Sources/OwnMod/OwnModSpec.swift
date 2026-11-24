import Foundation

struct OwnModSpec: Equatable {
    let folder: URL
    let baseGameID: GameID

    var userDataSubdirName: String {
        folder.path.split(separator: "/").joined(separator: "_")
    }
}

enum OwnModState: Equatable {
    case empty
    case selected(OwnModSpec, issues: [OwnModIssue])

    var spec: OwnModSpec? {
        guard case .selected(let spec, _) = self else { return nil }
        return spec
    }

    var issues: [OwnModIssue] {
        guard case .selected(_, let issues) = self else { return [] }
        return issues
    }

    var blockingIssue: OwnModIssue? {
        issues.first { $0.severity == .blocking }
    }

    var isLaunchable: Bool {
        spec != nil && blockingIssue == nil
    }
}

enum OwnModLaunchArguments {
    static func arguments(for spec: OwnModSpec) -> [String] {
        ["-mod", spec.folder.path]
    }
}

enum OwnModLaunchFailure: Error {
    case folderNotChosen
    case rejected(OwnModIssue)

    var analyticsReason: String {
        switch self {
        case .folderNotChosen: return "own_mod_folder_not_chosen"
        case .rejected(let issue): return "own_mod_\(issue.code)"
        }
    }

    var message: String {
        switch self {
        case .folderNotChosen: return OwnModText.folderNotChosen
        case .rejected(let issue): return issue.message
        }
    }
}
