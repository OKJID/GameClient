import SwiftUI

@main
struct GeneralsLauncherApp: App {
    var body: some Scene {
        WindowGroup {
            MainView()
                .frame(minWidth: 1040, idealWidth: 1200, minHeight: 660, idealHeight: 720)
                .edgesIgnoringSafeArea(.all)
        }
        .windowStyle(.hiddenTitleBar)
        .commands {
            CommandGroup(replacing: .appInfo) {
                Button(L10n.settings.aboutButton) {
                    AboutWindowController.show()
                }
            }
        }
    }
}
