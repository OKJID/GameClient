import SwiftUI

extension GameID {
    static let ownMod = GameID(rawValue: "own_mod")
}

extension GuideAnchor {
    static let ownMod = "own_mod"
}

extension LauncherTheme {
    static let ownMod = LauncherTheme(
        accent: Color(red: 0.16, green: 0.74, blue: 0.62),
        accentSoft: Color(red: 0.52, green: 0.92, blue: 0.80),
        panel: Color(red: 0.03, green: 0.12, blue: 0.11),
        panelBorder: Color(red: 0.12, green: 0.46, blue: 0.40),
        backgroundImageName: "background_mod"
    )
}

enum OwnModProfile {
    static let baseGameID = GameID.zeroHour

    static var profile: GameProfile {
        GameProfile.profile(for: baseGameID).derived(
            id: .ownMod,
            displayName: OwnModText.displayName,
            shortName: OwnModText.rowTitle,
            theme: .ownMod,
            kind: .ownMod,
            mod: nil
        )
    }
}

enum OwnModGuide {
    static var articleID: String? {
        GuideCatalog.shared.hint(anchor: GuideAnchor.ownMod)?.article
    }
}
