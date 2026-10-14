import SwiftUI
import AppKit

struct WorldBuilderButton: View {
    let accent: Color
    let iconName: String
    let title: String
    let description: String
    let action: () -> Void

    private let size: CGFloat = 57
    private let cornerRadius: CGFloat = 8
    private let tooltipWidth: CGFloat = 280
    private let tooltipGap: CGFloat = 2

    @State private var isHovering = false

    private var iconImage: NSImage? {
        guard let path = Bundle.main.path(forResource: iconName, ofType: "png") else {
            return nil
        }

        return NSImage(contentsOfFile: path)
    }

    var body: some View {
        Button(action: action) {
            face
                .frame(width: size, height: size)
                .shadow(color: accent.opacity(0.5), radius: 12)
        }
        .buttonStyle(PlainButtonStyle())
        .onHover { inside in
            isHovering = inside
            if inside { NSCursor.pointingHand.push() } else { NSCursor.pop() }
        }
        .overlay(alignment: .bottom) {
            if isHovering {
                tooltip
                    .offset(y: -(size + tooltipGap))
                    .allowsHitTesting(false)
            }
        }
    }

    @ViewBuilder
    private var face: some View {
        if let image = iconImage {
            Image(nsImage: image)
                .resizable()
                .aspectRatio(contentMode: .fit)
        } else {
            fallbackFace
        }
    }

    private var fallbackFace: some View {
        Image(systemName: "globe")
            .resizable()
            .aspectRatio(contentMode: .fit)
            .padding(14)
            .foregroundColor(.white)
            .frame(width: size, height: size)
            .background(
                LinearGradient(
                    colors: [accent.opacity(0.3), accent.opacity(0.15)],
                    startPoint: .leading,
                    endPoint: .trailing
                )
            )
            .overlay(RoundedRectangle(cornerRadius: cornerRadius).stroke(accent, lineWidth: 2))
            .clipShape(RoundedRectangle(cornerRadius: cornerRadius))
    }

    private var tooltip: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(title)
                .font(.system(size: 13, weight: .bold, design: .monospaced))
                .foregroundColor(accent)
            Text(description)
                .font(.system(size: 12, design: .monospaced))
                .foregroundColor(.white.opacity(0.85))
                .fixedSize(horizontal: false, vertical: true)
        }
        .padding(10)
        .frame(width: tooltipWidth, alignment: .leading)
        .background(RoundedRectangle(cornerRadius: 6).fill(Color.black.opacity(0.85)))
        .overlay(RoundedRectangle(cornerRadius: 6).stroke(accent.opacity(0.6), lineWidth: 1))
    }
}
