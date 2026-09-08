import SwiftUI
import AppKit

// MARK: - Palette

private enum P {
    static let ink       = Color.white
    static let money     = Color(red: 0.27, green: 0.85, blue: 0.53)
    static let moneyDeep = Color(red: 0.06, green: 0.41, blue: 0.24)
    static let noteEdge  = Color(red: 0.62, green: 0.96, blue: 0.75)
    static let noteInk   = Color(red: 0.09, green: 0.24, blue: 0.15)
}

// MARK: - Timeline

private enum Beat {
    static let idle: Double = 2.0
    static let flow: Double = 6.5
    static var total: Double { idle + flow }
}

// MARK: - Helpers

private func rnd(_ i: Double, _ a: Double, _ b: Double) -> Double {
    let s = sin(i * 12.9898 + 78.233) * 43758.5453
    return a + (b - a) * (s - s.rounded(.down))
}

private func ramp(_ t: Double, _ a: Double, _ b: Double) -> Double {
    min(max((t - a) / (b - a), 0), 1)
}

private func easeOutQuart(_ t: Double) -> Double { 1 - pow(1 - t, 4) }

// MARK: - Particle model

private struct Cash: Identifiable {
    let id: Int
    let start: Double
    let dur: Double
    let x: Double
    let drift: Double
    let rot: Double
    let spin: Double
    let scale: Double
    let sway: Double
    let isNote: Bool
    let amount: Int

    static let field: [Cash] = (0..<17).map { i in
        let d = Double(i)
        return Cash(
            id: i,
            start: (d / 17.0) * 4.2 + rnd(d + 5, 0, 0.18),
            dur: 1.85 + rnd(d + 90, 0, 0.45),
            x: (i % 2 == 0 ? -1 : 1) * (24 + rnd(d + 7, 0, 62)),
            drift: rnd(d + 31, -36, 36),
            rot: rnd(d + 53, -30, 30),
            spin: rnd(d + 71, -110, 110),
            scale: rnd(d + 17, 0.64, 1.1),
            sway: rnd(d + 44, 3, 10),
            isNote: i % 3 == 0,
            amount: [25, 50, 100, 200][i % 4]
        )
    }
}

// MARK: - One flying unit

private struct CashUnit: View {
    let c: Cash
    let p: Double
    let rise: Double

    var body: some View {
        let e = easeOutQuart(p)
        let sway = sin(p * 6 + c.rot) * c.sway
        let fade = min(1, p / 0.1) * (1 - max(0, (p - 0.6) / 0.4))

        Group {
            if c.isNote {
                Text("$")
                    .font(.system(size: 15, weight: .bold, design: .monospaced))
                    .foregroundColor(P.noteInk)
                    .frame(width: 30, height: 14)
                    .background(
                        RoundedRectangle(cornerRadius: 2)
                            .fill(LinearGradient(colors: [P.money, P.moneyDeep],
                                                 startPoint: .topLeading,
                                                 endPoint: .bottomTrailing))
                    )
                    .overlay(RoundedRectangle(cornerRadius: 2).stroke(P.noteEdge, lineWidth: 0.8))
                    .shadow(color: P.money.opacity(0.5), radius: 7)
                    .rotationEffect(.degrees(c.rot + c.spin * p))
            } else {
                Text("+$\(c.amount)")
                    .font(.system(size: 15, weight: .bold, design: .monospaced))
                    .foregroundColor(P.money)
                    .shadow(color: P.money.opacity(0.75), radius: 6)
            }
        }
        .scaleEffect(c.scale)
        .opacity(fade)
        .offset(x: c.x + c.drift * e + sway, y: -(34 + e * rise))
    }
}

// MARK: - Bar shape

private struct RightRoundedRectangle: Shape {
    let radius: CGFloat

    func path(in rect: CGRect) -> Path {
        Path { path in
            path.move(to: CGPoint(x: rect.minX, y: rect.minY))
            path.addLine(to: CGPoint(x: rect.maxX - radius, y: rect.minY))
            path.addArc(center: CGPoint(x: rect.maxX - radius, y: rect.minY + radius),
                        radius: radius,
                        startAngle: .degrees(-90),
                        endAngle: .degrees(0),
                        clockwise: false)
            path.addLine(to: CGPoint(x: rect.maxX, y: rect.maxY - radius))
            path.addArc(center: CGPoint(x: rect.maxX - radius, y: rect.maxY - radius),
                        radius: radius,
                        startAngle: .degrees(0),
                        endAngle: .degrees(90),
                        clockwise: false)
            path.addLine(to: CGPoint(x: rect.minX, y: rect.maxY))
            path.closeSubpath()
        }
    }
}

// MARK: - The button

struct SupportButton: View {
    let theme: LauncherTheme
    let label: String
    let animates: Bool
    let riseSpace: CGFloat
    let action: () -> Void

    private static let iconName = "hacker"

    private let iconSize: CGFloat = 44
    private let ringInset: CGFloat = 4
    private let ringWidth: CGFloat = 2
    private let barHeight: CGFloat = 22
    private let barCornerRadius: CGFloat = 6
    private let epoch = Date()
    private let ticker = Timer.publish(every: 1.0 / 60.0, on: .main, in: .common).autoconnect()

    @State private var beat: Double = 0

    init(theme: LauncherTheme,
         label: String,
         animates: Bool = true,
         riseSpace: CGFloat = 260,
         action: @escaping () -> Void = {}) {
        self.theme = theme
        self.label = label
        self.animates = animates
        self.riseSpace = riseSpace
        self.action = action
    }

    private var badgeSize: CGFloat { iconSize + (ringInset + ringWidth) * 2 }
    private var rim: Color { theme.accentSoft }
    private var fillTop: Color { theme.panelBorder }
    private var fillBot: Color { theme.panel }

    private var surface: LinearGradient {
        LinearGradient(colors: [fillTop.opacity(0.5), fillBot.opacity(0.5)],
                       startPoint: .top, endPoint: .bottom)
    }

    private var iconImage: NSImage? {
        guard let path = Bundle.main.path(forResource: SupportButton.iconName, ofType: "png") else {
            return nil
        }

        return NSImage(contentsOfFile: path)
    }

    private func advance(to now: Date) {
        guard animates else {
            return
        }

        beat = now.timeIntervalSince(epoch).truncatingRemainder(dividingBy: Beat.total)
    }

    var body: some View {
        frame(at: beat)
            .onReceive(ticker) { advance(to: $0) }
    }

    private func frame(at t: Double) -> some View {
        let loop = t / Beat.total
        let live = ramp(t, Beat.idle - 0.35, Beat.idle + 0.45)
                 * (1 - ramp(t, Beat.total - 1.0, Beat.total))
        let glow = 0.5 + 0.5 * sin(loop * .pi * 4)
        let scan = (loop * 6).truncatingRemainder(dividingBy: 1)

        return pill(live: live, glow: glow, scan: scan)
            .overlay(cashField(at: t), alignment: .bottom)
    }

    private func cashField(at t: Double) -> some View {
        ZStack(alignment: .bottom) {
            ForEach(Cash.field) { c in
                CashUnit(c: c,
                         p: min(max((t - (Beat.idle + c.start)) / c.dur, 0), 1),
                         rise: Double(riseSpace))
            }
        }
        .frame(height: badgeSize)
        .allowsHitTesting(false)
    }

    private func pill(live: Double, glow: Double, scan: Double) -> some View {
        Button(action: action) {
            ZStack(alignment: .bottomLeading) {
                labelBar(glow: glow, live: live)
                    .padding(.leading, badgeSize / 2)
                    .padding(.bottom, ringInset + ringWidth)
                avatarBadge(live: live, scan: scan)
            }
            .frame(height: badgeSize)
        }
        .buttonStyle(PlainButtonStyle())
        .onHover { inside in
            if inside { NSCursor.pointingHand.push() } else { NSCursor.pop() }
        }
    }

    private func avatarBadge(live: Double, scan: Double) -> some View {
        icon(live: live, scan: scan)
            .padding(ringInset + ringWidth)
            .background(Circle().fill(surface))
            .overlay(Circle().strokeBorder(rim.opacity(0.75), lineWidth: ringWidth))
    }

    private func labelBar(glow: Double, live: Double) -> some View {
        Text(label.uppercased())
            .font(.system(size: 13, weight: .bold, design: .monospaced))
            .kerning(1.0)
            .foregroundColor(rim)
            .lineLimit(1)
            .padding(.leading, badgeSize / 2 + 8)
            .padding(.trailing, 16)
            .frame(height: barHeight)
            .background(RightRoundedRectangle(radius: barCornerRadius).fill(surface))
            .overlay(RightRoundedRectangle(radius: barCornerRadius).stroke(rim.opacity(0.75), lineWidth: 1))
            .shadow(color: .black.opacity(0.45), radius: 6, y: 3)
            .shadow(color: rim.opacity(0.12 + glow * 0.05 + live * 0.14),
                    radius: 5 + glow * 3 + live * 4)
    }

    @ViewBuilder
    private var iconArtwork: some View {
        if let image = iconImage {
            Image(nsImage: image)
                .resizable()
                .aspectRatio(contentMode: .fill)
        } else {
            Image(systemName: "cup.and.saucer.fill")
                .resizable()
                .aspectRatio(contentMode: .fit)
                .padding(10)
                .foregroundColor(P.ink)
                .background(fillBot)
        }
    }

    private func icon(live: Double, scan: Double) -> some View {
        iconArtwork
            .frame(width: iconSize, height: iconSize)
            .saturation(0.9 + live * 0.5)
            .brightness(live * 0.06)
            .overlay(
                Rectangle()
                    .fill(P.money)
                    .frame(height: 1.5)
                    .shadow(color: P.money, radius: 5)
                    .opacity(live * 0.9)
                    .offset(y: (scan - 0.5) * Double(iconSize))
            )
            .overlay(
                LinearGradient(colors: [P.money.opacity(live * 0.18), .clear],
                               startPoint: .top, endPoint: .center)
            )
            .clipShape(Circle())
            .overlay(
                Circle().stroke(live > 0.45 ? P.money : rim, lineWidth: 1.2)
            )
            .shadow(color: (live > 0.45 ? P.money : rim).opacity(0.45),
                    radius: 5 + live * 5)
    }
}
