import AppKit

enum HotkeyKeyNames {
    static let noModifiers = "NONE"

    private static let keyPrefix = "KEY_"
    private static let escapeKeyCode: UInt16 = 0x35
    private static let clearKeyCodes: Set<UInt16> = [0x33, 0x75]
    private static let separator = " + "

    private static let engineKeysByKeyCode: [UInt16: String] = [
        0x00: "KEY_A", 0x01: "KEY_S", 0x02: "KEY_D", 0x03: "KEY_F", 0x04: "KEY_H", 0x05: "KEY_G",
        0x06: "KEY_Z", 0x07: "KEY_X", 0x08: "KEY_C", 0x09: "KEY_V", 0x0B: "KEY_B", 0x0C: "KEY_Q",
        0x0D: "KEY_W", 0x0E: "KEY_E", 0x0F: "KEY_R", 0x10: "KEY_Y", 0x11: "KEY_T", 0x12: "KEY_1",
        0x13: "KEY_2", 0x14: "KEY_3", 0x15: "KEY_4", 0x16: "KEY_6", 0x17: "KEY_5", 0x18: "KEY_EQUAL",
        0x19: "KEY_9", 0x1A: "KEY_7", 0x1B: "KEY_MINUS", 0x1C: "KEY_8", 0x1D: "KEY_0", 0x1E: "KEY_RBRACKET",
        0x1F: "KEY_O", 0x20: "KEY_U", 0x21: "KEY_LBRACKET", 0x22: "KEY_I", 0x23: "KEY_P", 0x24: "KEY_ENTER",
        0x25: "KEY_L", 0x26: "KEY_J", 0x27: "KEY_APOSTROPHE", 0x28: "KEY_K", 0x29: "KEY_SEMICOLON",
        0x2A: "KEY_BACKSLASH", 0x2B: "KEY_COMMA", 0x2C: "KEY_SLASH", 0x2D: "KEY_N", 0x2E: "KEY_M",
        0x2F: "KEY_PERIOD", 0x30: "KEY_TAB", 0x31: "KEY_SPACE", 0x32: "KEY_TICK", 0x33: "KEY_BACKSPACE",
        0x35: "KEY_ESC", 0x4C: "KEY_ENTER",
        0x7A: "KEY_F1", 0x78: "KEY_F2", 0x63: "KEY_F3", 0x76: "KEY_F4", 0x60: "KEY_F5", 0x61: "KEY_F6",
        0x62: "KEY_F7", 0x64: "KEY_F8", 0x65: "KEY_F9", 0x6D: "KEY_F10", 0x67: "KEY_F11", 0x6F: "KEY_F12",
        0x7B: "KEY_LEFT", 0x7C: "KEY_RIGHT", 0x7D: "KEY_DOWN", 0x7E: "KEY_UP", 0x75: "KEY_DEL",
        0x73: "KEY_HOME", 0x77: "KEY_END", 0x74: "KEY_PGUP", 0x79: "KEY_PGDN"
    ]

    private static let keyTitles: [String: String] = [
        "KP0": "Num 0", "KP1": "Num 1", "KP2": "Num 2", "KP3": "Num 3", "KP4": "Num 4",
        "KP5": "Num 5", "KP6": "Num 6", "KP7": "Num 7", "KP8": "Num 8", "KP9": "Num 9",
        "KPDEL": "Num Del", "KPSTAR": "Num *", "KPMINUS": "Num -", "KPPLUS": "Num +",
        "KPENTER": "Num Enter", "KPSLASH": "Num /",
        "ESC": "Esc", "BACKSPACE": "⌫", "ENTER": "Enter", "SPACE": "Space", "TAB": "Tab",
        "MINUS": "-", "EQUAL": "=", "LBRACKET": "[", "RBRACKET": "]", "SEMICOLON": ";",
        "APOSTROPHE": "'", "TICK": "`", "BACKSLASH": "\\", "COMMA": ",", "PERIOD": ".", "SLASH": "/",
        "UP": "↑", "DOWN": "↓", "LEFT": "←", "RIGHT": "→",
        "HOME": "Home", "END": "End", "PGUP": "PgUp", "PGDN": "PgDn", "INS": "Ins", "DEL": "Del",
        "NONE": "—"
    ]

    private static let modifierTitles: [String: String] = [
        "CTRL": "Ctrl", "ALT": "Alt", "SHIFT": "Shift"
    ]

    static func isEscape(_ event: NSEvent) -> Bool {
        event.keyCode == escapeKeyCode
    }

    static func isClear(_ event: NSEvent) -> Bool {
        clearKeyCodes.contains(event.keyCode)
    }

    static func engineKey(of event: NSEvent) -> String? {
        engineKeysByKeyCode[event.keyCode]
    }

    static func letter(of event: NSEvent) -> String? {
        guard let letter = engineKey(of: event).flatMap(HotkeyConflictFinder.letter(ofKey:)),
              letter.first?.isLetter == true else {
            return nil
        }

        return letter
    }

    static func modifiers(of flags: NSEvent.ModifierFlags) -> String {
        switch (flags.contains(.shift), flags.contains(.option), flags.contains(.control)) {
        case (false, false, false): return noModifiers
        case (true, false, false): return "SHIFT"
        case (false, true, false): return "ALT"
        case (false, false, true): return "CTRL"
        case (false, true, true): return "CTRL_ALT"
        case (true, false, true): return "SHIFT_CTRL"
        case (true, true, false): return "SHIFT_ALT"
        case (true, true, true): return "SHIFT_ALT_CTRL"
        }
    }

    static func title(ofChord chord: HotkeyEdits.GlobalKeyEdit) -> String {
        let modifiers = chord.modifiers == noModifiers ? [] : chord.modifiers.split(separator: "_").map(String.init)
        let parts = modifiers.map { modifierTitles[$0] ?? $0 } + [title(ofKey: chord.key)]
        return parts.joined(separator: separator)
    }

    static func title(ofKey key: String) -> String {
        let name = key.hasPrefix(keyPrefix) ? String(key.dropFirst(keyPrefix.count)) : key
        return keyTitles[name] ?? name
    }
}
