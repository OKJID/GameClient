import AppKit
import Combine

final class HotkeyEditorModel: ObservableObject {
    enum Phase {
        case idle
        case refreshing
        case applying
        case failed
    }

    enum Target: Hashable {
        case label(String)
        case global(String)
    }

    let files: HotkeyFiles
    private let launch: HotkeyEngineLaunch?
    private let isMapScrollOnWASD: Bool

    @Published private(set) var dump: HotkeyDump?
    @Published private(set) var labelUsage: HotkeyLabelUsage?
    @Published private(set) var edits: HotkeyEdits
    @Published private(set) var report = HotkeyConflictReport()
    @Published private(set) var sections: [HotkeySection] = []
    @Published private(set) var phase: Phase = .idle
    @Published private(set) var hasOriginals = false
    @Published private(set) var capturingTarget: Target?
    @Published var selectedSectionID: String?
    @Published var searchText = ""

    private var keyMonitor: Any?

    init(files: HotkeyFiles, launch: HotkeyEngineLaunch?, isMapScrollOnWASD: Bool) {
        self.files = files
        self.launch = launch
        self.isMapScrollOnWASD = isMapScrollOnWASD
        self.edits = files.loadEdits()
        reloadDump()
    }

    deinit {
        guard let keyMonitor else { return }
        NSEvent.removeMonitor(keyMonitor)
    }

    var hasEdits: Bool { !edits.isEmpty }
    var isBusy: Bool { phase == .refreshing || phase == .applying }

    var keymap: HotkeyKeymap? {
        dump.map { HotkeyKeymap(dump: $0, edits: edits) }
    }

    func select(_ section: HotkeySection) {
        selectedSectionID = section.id
        searchText = ""
    }

    func refresh() {
        guard !isBusy else { return }
        guard let launch else {
            phase = .failed
            return
        }

        phase = .refreshing
        HotkeyEngineRunner.dump(launch, files: files) { [weak self] isDumpFresh in
            self?.finishEngineRun(isDumpFresh: isDumpFresh)
        }
    }

    func apply() {
        guard hasEdits, !isBusy, let launch else { return }

        stopCapture()
        phase = .applying
        HotkeyEngineRunner.apply(launch, files: files) { [weak self] isDumpFresh in
            guard let self else { return }
            if isDumpFresh {
                self.replaceEdits(with: HotkeyEdits())
            }
            self.finishEngineRun(isDumpFresh: isDumpFresh)
        }
    }

    func reset() {
        guard !isBusy else { return }

        stopCapture()
        if let dump {
            files.restoreOriginals(of: dump)
        }
        replaceEdits(with: HotkeyEdits())
        refresh()
    }

    func setLetter(_ letter: String, forLabel label: String) {
        guard let original = dump?.hotkeys[label]?.key else { return }

        var updated = edits
        updated.hotkeys[label] = letter == original ? nil : letter
        replaceEdits(with: updated)
    }

    func setChord(_ chord: HotkeyEdits.GlobalKeyEdit, forGlobal id: String) {
        guard let dump else { return }

        var updated = edits
        for linked in HotkeyTree.linkedGlobals(of: id, in: dump) {
            guard let global = dump.globals[linked] else { continue }
            let original = HotkeyEdits.GlobalKeyEdit(key: global.key, modifiers: global.modifiers)
            updated.globals[linked] = chord == original ? nil : chord
        }
        replaceEdits(with: updated)
    }

    func revert(_ target: Target) {
        var updated = edits
        switch target {
        case .label(let label):
            updated.hotkeys[label] = nil
        case .global(let id):
            let linked = dump.map { HotkeyTree.linkedGlobals(of: id, in: $0) } ?? [id]
            linked.forEach { updated.globals[$0] = nil }
        }
        replaceEdits(with: updated)
    }

    func toggleCapture(_ target: Target) {
        guard phase != .applying else { return }
        guard capturingTarget != target else {
            stopCapture()
            return
        }

        stopCapture()
        capturingTarget = target
        keyMonitor = NSEvent.addLocalMonitorForEvents(matching: .keyDown) { [weak self] event in
            guard let self, self.handleCapturedKey(event) else { return event }
            return nil
        }
    }

    func stopCapture() {
        capturingTarget = nil
        guard let keyMonitor else { return }

        NSEvent.removeMonitor(keyMonitor)
        self.keyMonitor = nil
    }

    private func handleCapturedKey(_ event: NSEvent) -> Bool {
        guard let target = capturingTarget else { return false }
        guard !HotkeyKeyNames.isEscape(event) else {
            stopCapture()
            return true
        }

        switch target {
        case .label(let label):
            captureLetter(event, forLabel: label)
        case .global(let id):
            captureChord(event, forGlobal: id)
        }
        return true
    }

    private func captureLetter(_ event: NSEvent, forLabel label: String) {
        if HotkeyKeyNames.isClear(event) {
            setLetter("", forLabel: label)
            stopCapture()
            return
        }

        guard let letter = HotkeyKeyNames.letter(of: event) else { return }
        setLetter(letter, forLabel: label)
        stopCapture()
    }

    private func captureChord(_ event: NSEvent, forGlobal id: String) {
        guard let key = HotkeyKeyNames.engineKey(of: event) else { return }

        let modifiers = HotkeyKeyNames.modifiers(of: event.modifierFlags)
        setChord(HotkeyEdits.GlobalKeyEdit(key: key, modifiers: modifiers), forGlobal: id)
        stopCapture()
    }

    private func finishEngineRun(isDumpFresh: Bool) {
        guard isDumpFresh else {
            phase = .failed
            return
        }

        phase = .idle
        reloadDump()
    }

    private func replaceEdits(with updated: HotkeyEdits) {
        edits = updated
        files.saveEdits(updated)
        rebuildReport()
    }

    private func reloadDump() {
        dump = files.loadDump()
        hasOriginals = dump.map(files.hasOriginals) ?? false
        labelUsage = dump.map(HotkeyLabelUsage.init(dump:))
        sections = buildSections()
        selectFirstSectionIfNeeded()
        rebuildReport()
    }

    private func buildSections() -> [HotkeySection] {
        guard let dump, let labelUsage else { return [] }

        return HotkeyTree.sections(
            of: dump,
            usage: labelUsage,
            globalsTitle: L10n.hotkeys.globals,
            sharedTitle: L10n.hotkeys.shared,
            factionTitle: L10n.hotkeys.faction
        )
    }

    private func selectFirstSectionIfNeeded() {
        guard selectedSection == nil else { return }

        let firstFaction = sections.first { if case .faction = $0.kind { return true } else { return false } }
        selectedSectionID = (firstFaction ?? sections.first)?.id
    }

    private func rebuildReport() {
        guard let keymap else {
            report = HotkeyConflictReport()
            return
        }

        report = HotkeyConflictFinder.report(keymap: keymap, isMapScrollOnWASD: isMapScrollOnWASD)
    }
}
