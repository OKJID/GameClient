import SwiftUI
import AppKit

struct SupportSheet: View {
    let crash: PendingCrash?
    let accent: Color

    @Environment(\.dismiss) private var dismiss

    @State private var message = ""
    @State private var telegram = ""
    @State private var discord = ""
    @State private var email = ""
    @State private var attachment: URL?
    @State private var isAttachmentRejected = false
    @State private var attachLogs: Bool
    @State private var attachReplay = false
    @State private var isSending = false
    @State private var outcome: SubmissionOutcome?

    init(crash: PendingCrash?, accent: Color) {
        self.crash = crash
        self.accent = accent
        _attachLogs = State(initialValue: crash == nil)
    }

    private let sheetWidth: CGFloat = 480
    private let bytesPerMegabyte = 1024 * 1024
    private let logFileNames = "MacDebug.txt · GeneralsOnline.log · MacCrash.txt"
    private let lastReplayPath = "Replays/00000000.rep"
    private let recentWindow: TimeInterval = 12 * 60 * 60
    private let panelBackground = Color(red: 0.07, green: 0.07, blue: 0.08)
    private let errorColor = Color(red: 1.0, green: 0.4, blue: 0.35)

    private var isCrashReport: Bool { crash != nil }
    private var logsProfile: GameProfile { crash?.profile ?? GameProfile.current }
    private var lastReplayURL: URL { logsProfile.userDataDirURL.appendingPathComponent(lastReplayPath) }
    private var trimmedMessage: String { message.trimmingCharacters(in: .whitespacesAndNewlines) }

    private var lastReplayDate: Date? {
        let attributes = try? FileManager.default.attributesOfItem(atPath: lastReplayURL.path)
        return attributes?[.modificationDate] as? Date
    }

    private var canSend: Bool { !isSending && (isCrashReport || !trimmedMessage.isEmpty) }

    private var logFiles: [URL] {
        if attachLogs {
            return LogsSharer.existingLogs(in: logsProfile.userDataDirURL)
        }

        guard let crash else { return [] }
        return LogsSharer.existingCrashReports(in: crash.profile.userDataDirURL)
    }

    private var request: SupportRequest {
        SupportRequest(
            kind: isCrashReport ? .crash : .support,
            message: trimmedMessage,
            telegram: telegram.trimmingCharacters(in: .whitespacesAndNewlines),
            discord: discord.trimmingCharacters(in: .whitespacesAndNewlines),
            email: email.trimmingCharacters(in: .whitespacesAndNewlines),
            game: logsProfile.displayName,
            logFiles: logFiles,
            logsArchivePrefix: attachLogs ? logsProfile.logsArchivePrefix : SupportRequest.crashArchivePrefix(of: logsProfile),
            replay: attachReplay && lastReplayDate != nil ? lastReplayURL : nil,
            attachment: attachment
        )
    }

    private var failureMessage: String? {
        switch outcome {
        case .failed: return L10n.support.failed
        case .throttled: return L10n.support.throttled
        case .sent, nil: return nil
        }
    }

    private var attachmentLimitText: String {
        String(format: L10n.support.attachLimit, SupportSender.attachmentLimitMB)
    }

    private func send() {
        guard canSend else { return }

        isSending = true
        outcome = nil
        SupportSender.send(request) { result in
            isSending = false
            outcome = result
            markCrashReportedIfSent(result)
        }
    }

    private func markCrashReportedIfSent(_ result: SubmissionOutcome) {
        guard result == .sent, let crash else { return }
        CrashReports.markReported(crash)
    }

    private func chooseAttachment() {
        let panel = NSOpenPanel()
        panel.canChooseFiles = true
        panel.canChooseDirectories = false
        panel.allowsMultipleSelection = false

        guard panel.runModal() == .OK, let url = panel.url else { return }

        let size = (try? url.resourceValues(forKeys: [.fileSizeKey]))?.fileSize ?? 0
        isAttachmentRejected = size > SupportSender.attachmentLimitMB * bytesPerMegabyte
        attachment = isAttachmentRejected ? nil : url
    }

    private func lastReplayHint(now: Date) -> String {
        guard let lastReplayDate else {
            return "\(L10n.support.noReplay) · \(lastReplayPath)"
        }

        let played = String(format: L10n.support.attachReplayHint, formattedDate(lastReplayDate, now: now))
        return "\(played) · \(lastReplayPath)"
    }

    private func crashSubtitle(now: Date) -> String {
        guard let crash else { return "" }

        let crashedAt = Date(timeIntervalSince1970: crash.crashedAt)
        return String(format: L10n.support.crashSubtitle, formattedDate(crashedAt, now: now))
    }

    private func formattedDate(_ date: Date, now: Date) -> String {
        let formatter = DateFormatter()
        formatter.locale = Locale(identifier: L10n.current)
        formatter.dateStyle = .medium
        formatter.timeStyle = .short
        let dateText = formatter.string(from: date)

        let elapsed = now.timeIntervalSince(date)
        guard elapsed >= 0, elapsed < recentWindow else { return dateText }

        return "\(dateText) (\(String(format: L10n.support.ago, elapsedText(elapsed))))"
    }

    private func elapsedText(_ elapsed: TimeInterval) -> String {
        var calendar = Calendar.current
        calendar.locale = Locale(identifier: L10n.current)

        let formatter = DateComponentsFormatter()
        formatter.calendar = calendar
        formatter.allowedUnits = [.hour, .minute, .second]
        formatter.unitsStyle = .abbreviated
        return formatter.string(from: elapsed) ?? ""
    }

    private func sizeText(of url: URL) -> String {
        let size = (try? url.resourceValues(forKeys: [.fileSizeKey]))?.fileSize ?? 0
        return ByteCountFormatter.string(fromByteCount: Int64(size), countStyle: .file)
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
            _buildMessageField()
            _buildContactFields()
            _buildAttachmentRow()
            _buildLogsToggle()
            _buildReplayToggle()

            if let failureMessage {
                _buildError(failureMessage)
            }

            HStack(spacing: 10) {
                Spacer()

                _buildButton(title: isCrashReport ? L10n.alerts.close : L10n.alerts.cancel, isPrimary: false, action: { dismiss() })
                    .keyboardShortcut(.cancelAction)
                    .disabled(isSending)

                _buildSendButton()
            }
        }
    }

    private func _buildHeader() -> some View {
        VStack(alignment: .leading, spacing: 6) {
            Text(isCrashReport ? L10n.support.crashTitle : L10n.support.title)
                .font(.system(size: 14, weight: .black, design: .monospaced))
                .foregroundColor(.white)

            TimelineView(.periodic(from: .now, by: 1)) { context in
                Text(isCrashReport ? crashSubtitle(now: context.date) : L10n.support.subtitle)
                    .font(.system(size: 11, design: .monospaced))
                    .foregroundColor(.white.opacity(0.6))
                    .fixedSize(horizontal: false, vertical: true)
            }
        }
    }

    private func _buildMessageField() -> some View {
        VStack(alignment: .leading, spacing: 6) {
            _buildLabel(isCrashReport ? L10n.support.crashMessageLabel : L10n.support.messageLabel)

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

    private func _buildContactFields() -> some View {
        VStack(alignment: .leading, spacing: 6) {
            _buildLabel(L10n.support.contactsLabel)

            HStack(spacing: 8) {
                _buildTextField(placeholder: "Telegram", text: $telegram)
                _buildTextField(placeholder: "Discord", text: $discord)
                _buildTextField(placeholder: "E-mail", text: $email)
            }
        }
    }

    private func _buildAttachmentRow() -> some View {
        VStack(alignment: .leading, spacing: 6) {
            HStack(spacing: 10) {
                _buildButton(title: L10n.support.attachFile, isPrimary: false, action: chooseAttachment)
                    .disabled(isSending)

                if let attachment {
                    _buildAttachedFile(attachment)
                } else {
                    Text(attachmentLimitText)
                        .font(.system(size: 10, design: .monospaced))
                        .foregroundColor(.white.opacity(0.45))
                }
            }

            if isAttachmentRejected {
                _buildError(String(format: L10n.support.fileTooLarge, SupportSender.attachmentLimitMB))
            }
        }
    }

    private func _buildAttachedFile(_ url: URL) -> some View {
        HStack(spacing: 6) {
            Text("\(url.lastPathComponent) · \(sizeText(of: url))")
                .font(.system(size: 11, design: .monospaced))
                .foregroundColor(.white.opacity(0.8))
                .lineLimit(1)
                .truncationMode(.middle)

            Button(action: { attachment = nil }) {
                Image(systemName: "xmark.circle.fill")
                    .foregroundColor(.white.opacity(0.5))
            }
            .buttonStyle(PlainButtonStyle())
            .disabled(isSending)
        }
    }

    private func _buildLogsToggle() -> some View {
        _buildCheckbox(isOn: $attachLogs, title: L10n.support.attachLogs, hint: logFileNames)
            .disabled(isSending)
    }

    private func _buildReplayToggle() -> some View {
        TimelineView(.periodic(from: .now, by: 1)) { context in
            _buildCheckbox(isOn: $attachReplay, title: L10n.support.attachReplay, hint: lastReplayHint(now: context.date))
        }
        .disabled(isSending || lastReplayDate == nil)
    }

    private func _buildCheckbox(isOn: Binding<Bool>, title: String, hint: String) -> some View {
        Toggle(isOn: isOn) {
            VStack(alignment: .leading, spacing: 2) {
                Text(title)
                    .font(.system(size: 12, weight: .bold, design: .monospaced))
                    .foregroundColor(.white)

                Text(hint)
                    .font(.system(size: 10, design: .monospaced))
                    .foregroundColor(.white.opacity(0.45))
                    .fixedSize(horizontal: false, vertical: true)
            }
        }
        .toggleStyle(.checkbox)
    }

    private func _buildLabel(_ text: String) -> some View {
        Text(text)
            .font(.system(size: 9, weight: .bold, design: .monospaced))
            .foregroundColor(.white.opacity(0.5))
    }

    private func _buildError(_ text: String) -> some View {
        Text(text)
            .font(.system(size: 11, design: .monospaced))
            .foregroundColor(errorColor)
            .fixedSize(horizontal: false, vertical: true)
    }

    private func _buildTextField(placeholder: String, text: Binding<String>) -> some View {
        TextField(placeholder, text: text)
            .textFieldStyle(PlainTextFieldStyle())
            .font(.system(size: 12, design: .monospaced))
            .foregroundColor(.white)
            .padding(.horizontal, 8)
            .padding(.vertical, 7)
            .background(Color.black.opacity(0.5))
            .overlay(RoundedRectangle(cornerRadius: 4).stroke(accent.opacity(0.3), lineWidth: 1))
            .clipShape(RoundedRectangle(cornerRadius: 4))
            .disabled(isSending)
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

                Text(L10n.support.send)
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

            Text(isCrashReport ? L10n.support.crashSentTitle : L10n.support.sentTitle)
                .font(.system(size: 14, weight: .black, design: .monospaced))
                .foregroundColor(.white)

            Text(isCrashReport ? L10n.support.crashSentMsg : L10n.support.sentMsg)
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
