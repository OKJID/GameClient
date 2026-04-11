import SwiftUI

@main
struct GeneralsLauncherApp: App {
    var body: some Scene {
        WindowGroup {
            MainView()
                .frame(minWidth: 800, idealWidth: 800, minHeight: 500, idealHeight: 500)
                .edgesIgnoringSafeArea(.all)
        }
        .windowStyle(.hiddenTitleBar)
    }
}
