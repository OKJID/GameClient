import SwiftUI

struct ModRequestSheet: View {
    let accent: Color

    @Environment(\.dismiss) private var dismiss

    @State private var name = ""
    @State private var link = ""
    @State private var isSending = false
    @State private var outcome: SubmissionOutcome?

    private let sheetWidth: CGFloat = 420
    private let linkPlaceholder = "https://www.moddb.com/mods/…"
    private let panelBackground = Color(red: 0.07, green: 0.07, blue: 0.08)
    private let errorColor = Color(red: 1.0, green: 0.4, blue: 0.35)

    private var trimmedName: String { name.trimmingCharacters(in: .whitespacesAndNewlines) }
    private var trimmedLink: String { link.trimmingCharacters(in: .whitespacesAndNewlines) }
    private var canSend: Bool { !trimmedName.isEmpty && !isSending }

    private var failureMessage: String? {
        switch outcome {
        case .failed: return L10n.mod.request.failed
        case .throttled: return L10n.mod.request.throttled
        case .sent, nil: return nil
        }
    }

    private func send() {
        guard canSend else { return }

        isSending = true
        outcome = nil
        ModRequestSender.send(name: trimmedName, link: trimmedLink) { result in
            isSending = false
            outcome = result
        }
    }

    var body: some View {
        Group {
            if outcome == .sent {
                _buildConfirmation()
            } else {
                _buildForm()
            }
        }
        .padding(24)
        .frame(width: sheetWidth)
        .background(panelBackground)
    }

    private func _buildForm() -> some View {
        VStack(alignment: .leading, spacing: 16) {
            VStack(alignment: .leading, spacing: 6) {
                Text(L10n.mod.request.title)
                    .font(.system(size: 14, weight: .black, design: .monospaced))
                    .foregroundColor(.white)

                Text(L10n.mod.request.subtitle)
                    .font(.system(size: 11, design: .monospaced))
                    .foregroundColor(.white.opacity(0.6))
                    .fixedSize(horizontal: false, vertical: true)
            }

            _buildField(label: L10n.mod.request.nameLabel, placeholder: L10n.mod.request.namePlaceholder, text: $name)
            _buildField(label: L10n.mod.request.linkLabel, placeholder: linkPlaceholder, text: $link)

            if let failureMessage {
                Text(failureMessage)
                    .font(.system(size: 11, design: .monospaced))
                    .foregroundColor(errorColor)
                    .fixedSize(horizontal: false, vertical: true)
            }

            HStack(spacing: 10) {
                Spacer()

                _buildButton(title: L10n.alerts.cancel, isPrimary: false, action: { dismiss() })
                    .keyboardShortcut(.cancelAction)

                _buildSendButton()
            }
        }
    }

    private func _buildField(label: String, placeholder: String, text: Binding<String>) -> some View {
        VStack(alignment: .leading, spacing: 6) {
            Text(label)
                .font(.system(size: 9, weight: .bold, design: .monospaced))
                .foregroundColor(.white.opacity(0.5))

            TextField(placeholder, text: text)
                .onSubmit(send)
                .textFieldStyle(PlainTextFieldStyle())
                .font(.system(size: 13, design: .monospaced))
                .foregroundColor(.white)
                .padding(.horizontal, 10)
                .padding(.vertical, 7)
                .background(Color.black.opacity(0.5))
                .overlay(RoundedRectangle(cornerRadius: 4).stroke(accent.opacity(0.3), lineWidth: 1))
                .clipShape(RoundedRectangle(cornerRadius: 4))
                .disabled(isSending)
        }
    }

    private func _buildSendButton() -> some View {
        Button(action: send) {
            HStack(spacing: 6) {
                if isSending {
                    ProgressView()
                        .controlSize(.small)
                        .frame(width: 13, height: 13)
                } else {
                    Image(systemName: "paperplane.fill")
                        .font(.system(size: 11))
                }

                Text(L10n.mod.request.send)
                    .font(.system(size: 12, weight: .bold, design: .monospaced))
            }
            .foregroundColor(canSend ? .white : .white.opacity(0.3))
            .padding(.horizontal, 16)
            .padding(.vertical, 8)
            .background(canSend ? accent.opacity(0.25) : Color.white.opacity(0.05))
            .overlay(RoundedRectangle(cornerRadius: 4).stroke(canSend ? accent : Color.white.opacity(0.1), lineWidth: 1))
            .clipShape(RoundedRectangle(cornerRadius: 4))
        }
        .buttonStyle(PlainButtonStyle())
        .disabled(!canSend)
        .keyboardShortcut(.defaultAction)
    }

    private func _buildButton(title: String, isPrimary: Bool, action: @escaping () -> Void) -> some View {
        Button(action: action) {
            Text(title)
                .textCase(.uppercase)
                .font(.system(size: 12, weight: .bold, design: .monospaced))
                .foregroundColor(.white.opacity(0.8))
                .padding(.horizontal, 16)
                .padding(.vertical, 8)
                .background(isPrimary ? accent.opacity(0.25) : Color.white.opacity(0.05))
                .overlay(RoundedRectangle(cornerRadius: 4).stroke(isPrimary ? accent : Color.white.opacity(0.15), lineWidth: 1))
                .clipShape(RoundedRectangle(cornerRadius: 4))
        }
        .buttonStyle(PlainButtonStyle())
    }

    private func _buildConfirmation() -> some View {
        VStack(spacing: 14) {
            Image(systemName: "checkmark.seal.fill")
                .font(.system(size: 34))
                .foregroundColor(accent)

            Text(L10n.mod.request.sentTitle)
                .font(.system(size: 14, weight: .black, design: .monospaced))
                .foregroundColor(.white)

            Text(L10n.mod.request.sentMsg)
                .font(.system(size: 12, design: .monospaced))
                .foregroundColor(.white.opacity(0.75))
                .multilineTextAlignment(.center)
                .fixedSize(horizontal: false, vertical: true)

            _buildButton(title: L10n.alerts.close, isPrimary: true, action: { dismiss() })
                .keyboardShortcut(.defaultAction)
                .padding(.top, 4)
        }
        .frame(maxWidth: .infinity)
    }
}
