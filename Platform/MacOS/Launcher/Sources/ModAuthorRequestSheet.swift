import SwiftUI

struct ModAuthorRequestSheet: View {
    let profile: GameProfile
    let accent: Color

    @Environment(\.dismiss) private var dismiss

    @State private var kind: ModAuthorRequestKind = .change
    @State private var contact = ""
    @State private var message = ""
    @State private var isSending = false
    @State private var outcome: SubmissionOutcome?

    private let sheetWidth: CGFloat = 440
    private let panelBackground = Color(red: 0.07, green: 0.07, blue: 0.08)
    private let errorColor = Color(red: 1.0, green: 0.4, blue: 0.35)

    private var trimmedContact: String { contact.trimmingCharacters(in: .whitespacesAndNewlines) }
    private var trimmedMessage: String { message.trimmingCharacters(in: .whitespacesAndNewlines) }
    private var isMessageRequired: Bool { kind == .change }
    private var messageLabel: String { isMessageRequired ? L10n.mod.author.changeLabel : L10n.mod.author.removeLabel }

    private var canSend: Bool {
        guard !isSending, !trimmedContact.isEmpty else { return false }
        return !isMessageRequired || !trimmedMessage.isEmpty
    }

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
        ModRequestSender.sendAuthorRequest(
            kind: kind,
            profile: profile,
            contact: trimmedContact,
            message: trimmedMessage
        ) { result in
            isSending = false
            outcome = result
        }
    }

    private func title(of kind: ModAuthorRequestKind) -> String {
        switch kind {
        case .change: return L10n.mod.author.tabChange
        case .removal: return L10n.mod.author.tabRemove
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
            _buildHeader()
            _buildKindPicker()
            _buildContactField()
            _buildMessageField()

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

    private func _buildHeader() -> some View {
        VStack(alignment: .leading, spacing: 6) {
            Text(L10n.mod.author.title)
                .font(.system(size: 14, weight: .black, design: .monospaced))
                .foregroundColor(.white)

            Text(profile.displayName)
                .font(.system(size: 12, weight: .bold, design: .monospaced))
                .foregroundColor(accent)

            Text(L10n.mod.author.subtitle)
                .font(.system(size: 11, design: .monospaced))
                .foregroundColor(.white.opacity(0.6))
                .fixedSize(horizontal: false, vertical: true)
        }
    }

    private func _buildKindPicker() -> some View {
        Picker("", selection: $kind) {
            ForEach(ModAuthorRequestKind.allCases) { kind in
                Text(title(of: kind)).tag(kind)
            }
        }
        .pickerStyle(.segmented)
        .labelsHidden()
        .disabled(isSending)
    }

    private func _buildContactField() -> some View {
        VStack(alignment: .leading, spacing: 6) {
            _buildLabel(L10n.mod.author.contactLabel)

            TextField(L10n.mod.author.contactPlaceholder, text: $contact)
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

    private func _buildMessageField() -> some View {
        VStack(alignment: .leading, spacing: 6) {
            _buildLabel(messageLabel)

            TextEditor(text: $message)
                .font(.system(size: 12, design: .monospaced))
                .foregroundColor(.white)
                .scrollContentBackground(.hidden)
                .padding(6)
                .frame(height: 110)
                .background(Color.black.opacity(0.5))
                .overlay(RoundedRectangle(cornerRadius: 4).stroke(accent.opacity(0.3), lineWidth: 1))
                .clipShape(RoundedRectangle(cornerRadius: 4))
                .disabled(isSending)
        }
    }

    private func _buildLabel(_ text: String) -> some View {
        Text(text)
            .font(.system(size: 9, weight: .bold, design: .monospaced))
            .foregroundColor(.white.opacity(0.5))
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

            Text(L10n.mod.author.sentTitle)
                .font(.system(size: 14, weight: .black, design: .monospaced))
                .foregroundColor(.white)

            Text(L10n.mod.author.sentMsg)
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
