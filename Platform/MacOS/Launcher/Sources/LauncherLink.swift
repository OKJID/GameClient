import Foundation

enum LauncherLink: Equatable {
    static let scheme = "launcher"

    case guide(String)
    case settings(SettingKey?)
    case hotkeys
    case tab(LauncherViewModel.Tab)
    case support

    init?(url: URL) {
        guard url.scheme?.lowercased() == Self.scheme, let target = url.host?.lowercased() else {
            return nil
        }

        let argument = url.pathComponents.dropFirst().first ?? ""
        guard let link = Self.link(target: target, argument: argument) else {
            return nil
        }

        self = link
    }

    private static func link(target: String, argument: String) -> LauncherLink? {
        switch target {
        case "guide":
            return argument.isEmpty ? nil : .guide(argument)
        case "settings":
            return .settings(SettingKey(rawValue: argument))
        case "hotkeys":
            return .hotkeys
        case "tab":
            return tab(named: argument).map(LauncherLink.tab)
        case "support":
            return .support
        default:
            return nil
        }
    }

    private static func tab(named name: String) -> LauncherViewModel.Tab? {
        switch name.lowercased() {
        case "steam": return .steam
        case "local": return .local
        default: return nil
        }
    }
}
