import Foundation

enum OwnModText {
    static var rowTitle: String { resolve("rowTitle") }
    static var displayName: String { resolve("displayName") }
    static var howItWorks: String { resolve("howItWorks") }
    static var chooseFolder: String { resolve("chooseFolder") }
    static var changeFolder: String { resolve("changeFolder") }
    static var pickerMessage: String { resolve("pickerMessage") }
    static var folderMissing: String { resolve("folderMissing") }
    static var folderNotChosen: String { resolve("folderNotChosen") }
    static var gibArchivesIgnored: String { resolve("gibArchivesIgnored") }

    private static func resolve(_ key: String) -> String {
        translations[L10n.current]?[key]
            ?? translations["en"]?[key]
            ?? key
    }

    private static let translations: [String: [String: String]] = [
        "en": [
            "rowTitle": "OWN MOD",
            "displayName": "Own mod",
            "howItWorks": "How it works",
            "chooseFolder": "Choose folder",
            "changeFolder": "Change folder",
            "pickerMessage": "Select the mod folder. A folder without a mod starts plain Zero Hour.",
            "folderMissing": "Folder not found: %@",
            "folderNotChosen": "Choose the mod folder first.",
            "gibArchivesIgnored": ".gib archives will not load (found: %d). If they are part of the mod, rename them to .big."
        ],
        "ru": [
            "rowTitle": "СВОЙ МОД",
            "displayName": "Свой мод",
            "howItWorks": "Как это работает",
            "chooseFolder": "Выбрать папку",
            "changeFolder": "Сменить папку",
            "pickerMessage": "Выберите папку мода. Папка без мода запустит обычный Zero Hour.",
            "folderMissing": "Папка не найдена: %@",
            "folderNotChosen": "Сначала выберите папку мода.",
            "gibArchivesIgnored": "Архивы .gib не загрузятся (найдено: %d). Если они часть мода — переименуйте их в .big."
        ],
        "uk": [
            "rowTitle": "СВІЙ МОД",
            "displayName": "Свій мод",
            "howItWorks": "Як це працює",
            "chooseFolder": "Вибрати папку",
            "changeFolder": "Змінити папку",
            "pickerMessage": "Виберіть папку мода. Папка без мода запустить звичайний Zero Hour.",
            "folderMissing": "Папку не знайдено: %@",
            "folderNotChosen": "Спершу виберіть папку мода.",
            "gibArchivesIgnored": "Архіви .gib не завантажаться (знайдено: %d). Якщо вони частина мода — перейменуйте їх на .big."
        ],
        "ar": [
            "rowTitle": "تعديل خاص",
            "displayName": "تعديل خاص",
            "howItWorks": "كيف يعمل",
            "chooseFolder": "اختيار مجلد",
            "changeFolder": "تغيير المجلد",
            "pickerMessage": "اختر مجلد التعديل. المجلد الذي لا يحتوي على تعديل يشغّل Zero Hour العادية.",
            "folderMissing": "المجلد غير موجود: %@",
            "folderNotChosen": "اختر مجلد التعديل أولًا.",
            "gibArchivesIgnored": "لن يتم تحميل أرشيفات .gib (العدد: %d). إذا كانت جزءًا من التعديل، فغيّر امتدادها إلى .big."
        ],
        "kk": [
            "rowTitle": "ӨЗ МОДЫ",
            "displayName": "Өз моды",
            "howItWorks": "Бұл қалай жұмыс істейді",
            "chooseFolder": "Қалтаны таңдау",
            "changeFolder": "Қалтаны ауыстыру",
            "pickerMessage": "Мод қалтасын таңдаңыз. Мод жоқ қалта кәдімгі Zero Hour-ды іске қосады.",
            "folderMissing": "Қалта табылмады: %@",
            "folderNotChosen": "Алдымен мод қалтасын таңдаңыз.",
            "gibArchivesIgnored": ".gib архивтері жүктелмейді (табылды: %d). Егер олар модтың бөлігі болса, кеңейтімін .big етіп өзгертіңіз."
        ],
        "vi": [
            "rowTitle": "MOD RIÊNG",
            "displayName": "Mod riêng",
            "howItWorks": "Cách hoạt động",
            "chooseFolder": "Chọn thư mục",
            "changeFolder": "Đổi thư mục",
            "pickerMessage": "Chọn thư mục mod. Thư mục không có mod sẽ chạy Zero Hour thường.",
            "folderMissing": "Không tìm thấy thư mục: %@",
            "folderNotChosen": "Hãy chọn thư mục mod trước.",
            "gibArchivesIgnored": "Các tệp lưu trữ .gib sẽ không được tải (tìm thấy: %d). Nếu chúng thuộc về mod, hãy đổi đuôi thành .big."
        ],
        "pl": [
            "rowTitle": "WŁASNY MOD",
            "displayName": "Własny mod",
            "howItWorks": "Jak to działa",
            "chooseFolder": "Wybierz folder",
            "changeFolder": "Zmień folder",
            "pickerMessage": "Wybierz folder moda. Folder bez moda uruchomi zwykłe Zero Hour.",
            "folderMissing": "Nie znaleziono folderu: %@",
            "folderNotChosen": "Najpierw wybierz folder moda.",
            "gibArchivesIgnored": "Archiwa .gib nie zostaną wczytane (znaleziono: %d). Jeśli są częścią moda, zmień ich rozszerzenie na .big."
        ],
        "de": [
            "rowTitle": "EIGENER MOD",
            "displayName": "Eigener Mod",
            "howItWorks": "So funktioniert es",
            "chooseFolder": "Ordner wählen",
            "changeFolder": "Ordner ändern",
            "pickerMessage": "Wähle den Mod-Ordner. Ein Ordner ohne Mod startet normales Zero Hour.",
            "folderMissing": "Ordner nicht gefunden: %@",
            "folderNotChosen": "Wähle zuerst den Mod-Ordner.",
            "gibArchivesIgnored": ".gib-Archive werden nicht geladen (gefunden: %d). Wenn sie zum Mod gehören, benenne sie in .big um."
        ],
        "es": [
            "rowTitle": "MOD PROPIO",
            "displayName": "Mod propio",
            "howItWorks": "Cómo funciona",
            "chooseFolder": "Elegir carpeta",
            "changeFolder": "Cambiar carpeta",
            "pickerMessage": "Elige la carpeta del mod. Una carpeta sin mod inicia Zero Hour normal.",
            "folderMissing": "Carpeta no encontrada: %@",
            "folderNotChosen": "Primero elige la carpeta del mod.",
            "gibArchivesIgnored": "Los archivos .gib no se cargarán (encontrados: %d). Si forman parte del mod, cámbiales la extensión a .big."
        ],
        "tr": [
            "rowTitle": "KENDİ MODUN",
            "displayName": "Kendi modun",
            "howItWorks": "Nasıl çalışır",
            "chooseFolder": "Klasör seç",
            "changeFolder": "Klasörü değiştir",
            "pickerMessage": "Mod klasörünü seçin. Mod içermeyen klasör normal Zero Hour'u başlatır.",
            "folderMissing": "Klasör bulunamadı: %@",
            "folderNotChosen": "Önce mod klasörünü seçin.",
            "gibArchivesIgnored": ".gib arşivleri yüklenmeyecek (bulunan: %d). Modun parçasıysalar uzantılarını .big olarak değiştirin."
        ],
        "zh": [
            "rowTitle": "自定义模组",
            "displayName": "自定义模组",
            "howItWorks": "使用说明",
            "chooseFolder": "选择文件夹",
            "changeFolder": "更换文件夹",
            "pickerMessage": "选择模组文件夹。不含模组的文件夹将启动普通的 Zero Hour。",
            "folderMissing": "找不到文件夹：%@",
            "folderNotChosen": "请先选择模组文件夹。",
            "gibArchivesIgnored": ".gib 档案不会被加载（找到 %d 个）。如果它们属于该模组，请将扩展名改为 .big。"
        ],
        "hi": [
            "rowTitle": "अपना मॉड",
            "displayName": "अपना मॉड",
            "howItWorks": "यह कैसे काम करता है",
            "chooseFolder": "फ़ोल्डर चुनें",
            "changeFolder": "फ़ोल्डर बदलें",
            "pickerMessage": "मॉड फ़ोल्डर चुनें। बिना मॉड वाला फ़ोल्डर सामान्य Zero Hour चलाता है।",
            "folderMissing": "फ़ोल्डर नहीं मिला: %@",
            "folderNotChosen": "पहले मॉड फ़ोल्डर चुनें।",
            "gibArchivesIgnored": ".gib आर्काइव लोड नहीं होंगे (मिले: %d)। अगर ये मॉड का हिस्सा हैं, तो इनका एक्सटेंशन .big कर दें।"
        ]
    ]
}
