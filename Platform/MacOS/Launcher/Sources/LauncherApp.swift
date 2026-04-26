import SwiftUI

@main
struct GeneralsLauncherApp: App {
    var body: some Scene {
        WindowGroup {
            MainView()
                .frame(minWidth: 860, idealWidth: 860, minHeight: 580, idealHeight: 580)
                .edgesIgnoringSafeArea(.all)
        }
        .windowStyle(.hiddenTitleBar)
        .commands {
            CommandGroup(replacing: .appInfo) {
                Button("About Generals Online") {
                    AboutWindowController.show()
                }
            }
        }
    }
}
