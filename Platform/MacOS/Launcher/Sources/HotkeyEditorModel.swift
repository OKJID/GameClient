import AppKit
import Combine

final class HotkeyEditorModel: ObservableObject {
    enum Phase {
        case idle
        case refreshing
        case applying
        case importing
        case exporting
        case failed
    }

    enum Target: Hashable {
        case label(String)
        case global(String)
    }

    let files: HotkeyFiles
    let presetsFolder: URL
    private let installRoot: String
    private let launch: HotkeyEngineLaunch?
    private let isMapScrollOnWASD: Bool
    let isCommandControl: Bool

    @Published private(set) var dump: HotkeyDump?
    @Published private(set) var labelUsage: HotkeyLabelUsage?
    @Published private(set) var edits: HotkeyEdits
    @Published private(set) var report = HotkeyConflictReport()
    @Published private(set) var sections: [HotkeySection] = []
    @Published private(set) var phase: Phase = .idle
    @Published private(set) var hasOriginals = false
    @Published private(set) var capturingTarget: Target?
    @Published private(set) var presets: [HotkeyPresetEntry] = []
    @Published private(set) var presetOutcome: HotkeyPresetOutcome?
    @Published private(set) var draftPresetName: String?
    @Published var selectedSectionID: String?
    @Published var searchText = ""

    private var keyMonitor: Any?
    private var presetDraftEdits: HotkeyEdits?
    private var savedPresetState: HotkeyPresetState?

    init(files: HotkeyFiles, presetsFolder: URL, installRoot: String, launch: HotkeyEngineLaunch?, isMapScrollOnWASD: Bool, isCommandControl: Bool) {
        self.files = files
        self.presetsFolder = presetsFolder
        self.installRoot = installRoot
        self.launch = launch
        self.isMapScrollOnWASD = isMapScrollOnWASD
        self.isCommandControl = isCommandControl
        self.edits = files.loadEdits()
        self.savedPresetState = files.loadPresetState()
        reloadDump()
    }

    deinit {
        guard let keyMonitor else { return }
        NSEvent.removeMonitor(keyMonitor)
    }

    var hasEdits: Bool { !edits.isEmpty }
    var isBusy: Bool { phase != .idle && phase != .failed }
    var shippedPresets: [HotkeyPresetEntry] { presets.filter(\.isShipped) }
    var ownPresets: [HotkeyPresetEntry] { presets.filter { !$0.isShipped } }
    var presetName: String? { draftPresetName ?? appliedPreset.name }

    var isPresetModified: Bool {
        guard draftPresetName != nil else { return hasEdits || appliedPreset.isModified }
        return edits != presetDraftEdits
    }

    var keymap: HotkeyKeymap? {
        dump.map { HotkeyKeymap(dump: $0, edits: edits) }
    }

    private var appliedPreset: HotkeyPresetState {
        guard let savedPresetState, savedPresetState.belongs(to: installRoot) else {
            return HotkeyPresetState(name: nil, isModified: hasOriginals)
        }

        return savedPresetState
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
        let appliedState = HotkeyPresetState(name: presetName, isModified: isPresetModified, root: installRoot)
        phase = .applying
        HotkeyEngineRunner.apply(launch, files: files) { [weak self] isDumpFresh in
            guard let self else { return }
            if isDumpFresh {
                self.replaceEdits(with: HotkeyEdits())
                self.rememberAppliedPreset(appliedState)
            }
            self.finishEngineRun(isDumpFresh: isDumpFresh)
        }
    }

    func discardEdits() {
        guard !isBusy else { return }

        stopCapture()
        replaceEdits(with: HotkeyEdits())
        forgetDraftPreset()
    }

    func reset() {
        guard !isBusy else { return }

        stopCapture()
        if let dump {
            files.restoreOriginals(of: dump)
        }
        let standardState = HotkeyPresetState(name: nil, isModified: false, root: installRoot)
        files.savePresetState(standardState)
        savedPresetState = standardState
        replaceEdits(with: HotkeyEdits())
        forgetDraftPreset()
        refresh()
    }

    func applyPreset(_ entry: HotkeyPresetEntry) {
        guard !isBusy else { return }

        stopCapture()
        switch entry.kind {
        case .shipped(let preset):
            mergeIntoDraft(preset, named: entry.name)
        case .own:
            readOwnPreset(entry)
        }
    }

    func importPreset(from source: URL) {
        guard !isBusy else { return }

        guard let copy = HotkeyPresetStore.addOwn(source, to: presetsFolder) else {
            presetOutcome = .importFailed(fileName: source.lastPathComponent)
            return
        }

        reloadPresets()
        applyPreset(HotkeyPresetEntry(url: copy, kind: .own))
    }

    func exportApplied(to destination: URL) {
        guard !isBusy else { return }
        guard let launch else {
            presetOutcome = .saveFailed(fileName: destination.lastPathComponent)
            return
        }

        stopCapture()
        phase = .exporting
        HotkeyEngineRunner.export(launch, to: destination) { [weak self] isWritten in
            guard let self else { return }
            self.phase = .idle
            self.reloadPresets()
            self.presetOutcome = isWritten
                ? .saved(fileName: destination.lastPathComponent)
                : .saveFailed(fileName: destination.lastPathComponent)
        }
    }

    @discardableResult
    func deleteOwnPreset(_ entry: HotkeyPresetEntry) -> Bool {
        guard !entry.isShipped, !isBusy else { return false }
        guard HotkeyPresetStore.moveOwnToTrash(entry.url) else {
            presetOutcome = .deleteFailed(fileName: entry.title)
            return false
        }

        reloadPresets()
        return true
    }

    func reloadPresets() {
        presets = dump.map { HotkeyPresetStore.entries(in: presetsFolder, for: $0.profile) } ?? []
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

        let modifiers = HotkeyKeyNames.modifiers(of: event.modifierFlags, isCommandControl: isCommandControl)
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

    private func mergeIntoDraft(_ preset: HotkeyPreset, named name: String) {
        guard let dump else { return }

        let merged = HotkeyPresetMatcher.merged(preset, into: edits, against: dump)
        replaceEdits(with: merged)
        draftPresetName = name
        presetDraftEdits = merged
    }

    private func readOwnPreset(_ entry: HotkeyPresetEntry) {
        guard let launch else {
            presetOutcome = .importFailed(fileName: entry.title)
            return
        }

        phase = .importing
        HotkeyEngineRunner.importPreset(launch, from: entry.url, to: files.importURL) { [weak self] isPresetFresh in
            self?.finishOwnPreset(entry, isPresetFresh: isPresetFresh)
        }
    }

    private func finishOwnPreset(_ entry: HotkeyPresetEntry, isPresetFresh: Bool) {
        phase = .idle
        let preset = isPresetFresh ? HotkeyPresetStore.load(files.importURL) : nil
        try? FileManager.default.removeItem(at: files.importURL)

        guard let preset else {
            presetOutcome = .importFailed(fileName: entry.title)
            return
        }

        mergeIntoDraft(preset, named: entry.name)
    }

    private func replaceEdits(with updated: HotkeyEdits) {
        edits = updated
        presetOutcome = nil
        files.saveEdits(updated)
        rebuildReport()
    }

    private func rememberAppliedPreset(_ state: HotkeyPresetState) {
        savedPresetState = state
        files.savePresetState(state)
        forgetDraftPreset()
    }

    private func forgetDraftPreset() {
        draftPresetName = nil
        presetDraftEdits = nil
    }

    private func reloadDump() {
        dump = files.loadDump()
        hasOriginals = dump.map(files.hasOriginals) ?? false
        labelUsage = dump.map(HotkeyLabelUsage.init(dump:))
        sections = buildSections()
        selectFirstSectionIfNeeded()
        reloadPresets()
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
