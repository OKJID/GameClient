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
            "folderNotChosen": "Choose the mod folder first."
        ],
        "ru": [
            "rowTitle": "СВОЙ МОД",
            "displayName": "Свой мод",
            "howItWorks": "Как это работает",
            "chooseFolder": "Выбрать папку",
            "changeFolder": "Сменить папку",
            "pickerMessage": "Выберите папку мода. Папка без мода запустит обычный Zero Hour.",
            "folderMissing": "Папка не найдена: %@",
            "folderNotChosen": "Сначала выберите папку мода."
        ],
        "uk": [
            "rowTitle": "СВІЙ МОД",
            "displayName": "Свій мод",
            "howItWorks": "Як це працює",
            "chooseFolder": "Вибрати теку",
            "changeFolder": "Змінити теку",
            "pickerMessage": "Виберіть теку мода. Тека без мода запустить звичайний Zero Hour.",
            "folderMissing": "Теку не знайдено: %@",
            "folderNotChosen": "Спершу виберіть теку мода."
        ],
        "ar": [
            "rowTitle": "تعديل خاص",
            "displayName": "تعديل خاص",
            "howItWorks": "كيف يعمل",
            "chooseFolder": "اختيار مجلد",
            "changeFolder": "تغيير المجلد",
            "pickerMessage": "اختر مجلد التعديل. المجلد الذي لا يحتوي على تعديل يشغّل Zero Hour العادية.",
            "folderMissing": "المجلد غير موجود: %@",
            "folderNotChosen": "اختر مجلد التعديل أولًا."
        ],
        "kk": [
            "rowTitle": "ӨЗ МОДЫ",
            "displayName": "Өз моды",
            "howItWorks": "Бұл қалай жұмыс істейді",
            "chooseFolder": "Буманы таңдау",
            "changeFolder": "Буманы ауыстыру",
            "pickerMessage": "Мод бумасын таңдаңыз. Мод жоқ бума кәдімгі Zero Hour-ды іске қосады.",
            "folderMissing": "Бума табылмады: %@",
            "folderNotChosen": "Алдымен мод бумасын таңдаңыз."
        ],
        "vi": [
            "rowTitle": "MOD RIÊNG",
            "displayName": "Mod riêng",
            "howItWorks": "Cách hoạt động",
            "chooseFolder": "Chọn thư mục",
            "changeFolder": "Đổi thư mục",
            "pickerMessage": "Chọn thư mục mod. Thư mục không có mod sẽ chạy Zero Hour thường.",
            "folderMissing": "Không tìm thấy thư mục: %@",
            "folderNotChosen": "Hãy chọn thư mục mod trước."
        ],
        "pl": [
            "rowTitle": "WŁASNY MOD",
            "displayName": "Własny mod",
            "howItWorks": "Jak to działa",
            "chooseFolder": "Wybierz folder",
            "changeFolder": "Zmień folder",
            "pickerMessage": "Wybierz folder moda. Folder bez moda uruchomi zwykłe Zero Hour.",
            "folderMissing": "Nie znaleziono folderu: %@",
            "folderNotChosen": "Najpierw wybierz folder moda."
        ],
        "de": [
            "rowTitle": "EIGENER MOD",
            "displayName": "Eigener Mod",
            "howItWorks": "So funktioniert es",
            "chooseFolder": "Ordner wählen",
            "changeFolder": "Ordner ändern",
            "pickerMessage": "Wählen Sie den Mod-Ordner. Ein Ordner ohne Mod startet normales Zero Hour.",
            "folderMissing": "Ordner nicht gefunden: %@",
            "folderNotChosen": "Wählen Sie zuerst den Mod-Ordner."
        ],
        "es": [
            "rowTitle": "MOD PROPIO",
            "displayName": "Mod propio",
            "howItWorks": "Cómo funciona",
            "chooseFolder": "Elegir carpeta",
            "changeFolder": "Cambiar carpeta",
            "pickerMessage": "Elige la carpeta del mod. Una carpeta sin mod inicia Zero Hour normal.",
            "folderMissing": "Carpeta no encontrada: %@",
            "folderNotChosen": "Primero elige la carpeta del mod."
        ],
        "tr": [
            "rowTitle": "KENDİ MODUN",
            "displayName": "Kendi modun",
            "howItWorks": "Nasıl çalışır",
            "chooseFolder": "Klasör seç",
            "changeFolder": "Klasörü değiştir",
            "pickerMessage": "Mod klasörünü seçin. Mod içermeyen klasör normal Zero Hour'u başlatır.",
            "folderMissing": "Klasör bulunamadı: %@",
            "folderNotChosen": "Önce mod klasörünü seçin."
        ],
        "zh": [
            "rowTitle": "自定义模组",
            "displayName": "自定义模组",
            "howItWorks": "使用说明",
            "chooseFolder": "选择文件夹",
            "changeFolder": "更换文件夹",
            "pickerMessage": "选择模组文件夹。不含模组的文件夹将启动普通的 Zero Hour。",
            "folderMissing": "找不到文件夹：%@",
            "folderNotChosen": "请先选择模组文件夹。"
        ],
        "hi": [
            "rowTitle": "अपना मॉड",
            "displayName": "अपना मॉड",
            "howItWorks": "यह कैसे काम करता है",
            "chooseFolder": "फ़ोल्डर चुनें",
            "changeFolder": "फ़ोल्डर बदलें",
            "pickerMessage": "मॉड फ़ोल्डर चुनें। बिना मॉड वाला फ़ोल्डर सामान्य Zero Hour चलाता है।",
            "folderMissing": "फ़ोल्डर नहीं मिला: %@",
            "folderNotChosen": "पहले मॉड फ़ोल्डर चुनें।"
        ]
    ]
}
