import SwiftUI
import AppKit

struct WindowAccessor: NSViewRepresentable {
    static let minimumContentSize = NSSize(width: 1040, height: 660)

    func makeNSView(context: Context) -> NSView {
        let view = NSView()
        DispatchQueue.main.async {
            guard let window = view.window else { return }
            window.titlebarAppearsTransparent = true
            window.titleVisibility = .hidden
            window.styleMask.insert(.fullSizeContentView)
            Self.enforceMinimumSize(on: window)
        }
        return view
    }

    func updateNSView(_ nsView: NSView, context: Context) {}

    private static func enforceMinimumSize(on window: NSWindow) {
        let minimum = window.frameRect(forContentRect: NSRect(origin: .zero, size: minimumContentSize)).size
        window.minSize = minimum

        guard window.frame.width < minimum.width || window.frame.height < minimum.height else { return }

        let target = NSSize(
            width: max(window.frame.width, minimum.width),
            height: max(window.frame.height, minimum.height)
        )
        window.setContentSize(window.contentRect(forFrameRect: NSRect(origin: .zero, size: target)).size)
        window.center()
    }
}
