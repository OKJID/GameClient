import Foundation

enum L10n {
    private static let langKey = "LAUNCHER_LANGUAGE"

    static var current: String = {
        if let saved = UserDefaults.standard.string(forKey: langKey),
           supportedLanguages.contains(saved) {
            return saved
        }
        let preferred = Locale.preferredLanguages.first ?? "en"
        let code = String(preferred.prefix(2))
        return supportedLanguages.contains(code) ? code : "en"
    }()

    static func setCurrent(_ lang: String) {
        guard supportedLanguages.contains(lang) else { return }
        current = lang
        UserDefaults.standard.set(lang, forKey: langKey)
    }

    static let supportedLanguages = [
        "en", "ru", "uk", "ar", "kk", "vi", "pl", "de", "es", "tr", "zh", "hi"
    ]

    static let languageNames: [String: String] = [
        "en": "English",
        "ru": "Русский",
        "uk": "Українська",
        "ar": "العربية",
        "kk": "Қазақша",
        "vi": "Tiếng Việt",
        "pl": "Polski",
        "de": "Deutsch",
        "es": "Español",
        "tr": "Türkçe",
        "zh": "中文",
        "hi": "हिन्दी"
    ]

    private static func resolve(_ key: String) -> String {
        translations[current]?[key]
            ?? translations["en"]?[key]
            ?? key
    }

    static let app = App()
    static let tab = Tab()
    static let steam = Steam()
    static let local = Local()
    static let action = Action()
    static let alerts = Alerts()
    static let update = Update()
    static let footer = Footer()
    static let patch = Patch()
    static let folder = Folder()

    struct App {
        var title: String { resolve("app.title") }
        var subtitle: String { resolve("app.subtitle") }
    }

    struct Tab {
        var steam: String { resolve("tab.steam") }
        var local: String { resolve("tab.local") }
    }

    struct Steam {
        var credentials: String { resolve("steam.credentials") }
        var submit: String { resolve("steam.submit") }
        var cancel: String { resolve("steam.cancel") }
        var download: String { resolve("steam.download") }
        let status = SteamStatus()
    }

    struct SteamStatus {
        var ready: String { resolve("steam.status.ready") }
        var installingSteamCMD: String { resolve("steam.status.installingSteamCMD") }
        var awaitingCreds: String { resolve("steam.status.awaitingCreds") }
        var authenticating: String { resolve("steam.status.authenticating") }
        var steamGuard: String { resolve("steam.status.steamGuard") }
        var downloading: String { resolve("steam.status.downloading") }
        var validating: String { resolve("steam.status.validating") }
        var downloadingPatch: String { resolve("steam.status.downloadingPatch") }
        var unpacking: String { resolve("steam.status.unpacking") }
        var assetsReady: String { resolve("steam.status.assetsReady") }
        var error: String { resolve("steam.status.error") }
    }

    struct Local {
        var path: String { resolve("local.path") }
        var locate: String { resolve("local.locate") }
        var selectHint: String { resolve("local.selectHint") }
        var invalidTarget: String { resolve("local.invalidTarget") }
    }

    struct Action {
        var launch: String { resolve("action.launch") }
        var patch: String { resolve("action.patch") }
        var initialize: String { resolve("action.init") }
    }

    struct Alerts {
        var launchError: String { resolve("alert.launchError") }
        var ok: String { resolve("alert.ok") }
        var gameNotFound: String { resolve("alert.gameNotFound") }
        var gameNotFoundMsg: String { resolve("alert.gameNotFoundMsg") }
        var openSteamStore: String { resolve("alert.openSteamStore") }
        var close: String { resolve("alert.close") }
        var patchTitle: String { resolve("alert.patchTitle") }
        var patchMsg: String { resolve("alert.patchMsg") }
        var patchButton: String { resolve("alert.patchButton") }
    }

    struct Update {
        var available: String { resolve("update.available") }
        var download: String { resolve("update.download") }
        var details: String { resolve("update.details") }
    }

    struct Footer {
        var author: String { resolve("footer.author") }
    }

    struct Patch {
        let status = PatchStatus()
    }

    struct PatchStatus {
        var idle: String { resolve("patch.status.idle") }
        var cleaning: String { resolve("patch.status.cleaning") }
        var downloading: String { resolve("patch.status.downloading") }
        var unpacking: String { resolve("patch.status.unpacking") }
        var completed: String { resolve("patch.status.completed") }
        var error: String { resolve("patch.status.error") }
    }

    struct Folder {
        var prompt: String { resolve("folder.prompt") }
    }

    static let translations: [String: [String: String]] = [
        "en": en, "ru": ru, "uk": uk, "ar": ar, "kk": kk,
        "vi": vi, "pl": pl, "de": de, "es": es, "tr": tr,
        "zh": zh, "hi": hi
    ]

    static let en: [String: String] = [
        "app.title": "GENERALS ONLINE",
        "app.subtitle": "COMMUNITY MAC PORT",
        "tab.steam": "STEAM (RECOMMENDED)",
        "tab.local": "LOCAL ARCHIVE",
        "steam.credentials": "STEAM CREDENTIALS",
        "steam.submit": "SUBMIT",
        "steam.cancel": "CANCEL",
        "steam.download": "DOWNLOAD ASSETS",
        "local.path": "TACTICAL DATA PATH:",
        "local.locate": "LOCATE",
        "local.selectHint": "SELECT THE PARENT DIRECTORY CONTAINING BOTH GAME VERSIONS",
        "local.invalidTarget": "INVALID TARGET — No ini.big / inizh.big detected in subdirectories",
        "action.launch": "LAUNCH",
        "action.patch": "PATCH",
        "action.init": "INITIALIZING...",
        "alert.launchError": "Launch Error",
        "alert.ok": "OK",
        "alert.gameNotFound": "Game Not Found",
        "alert.gameNotFoundMsg": "The account \"%@\" does not own Command & Conquer™ Generals — Zero Hour.\n\nPurchase the game on Steam, then press \"Download Assets\" again.",
        "alert.openSteamStore": "Open Steam Store",
        "alert.close": "Close",
        "alert.patchTitle": "Apply Community Patch?",
        "alert.patchMsg": "Your game files may be irreversibly modified or deleted.\nThis is required to play online.",
        "alert.patchButton": "Patch",
        "update.available": "UPDATE AVAILABLE: v%@",
        "update.download": "DOWNLOAD UPDATE",
        "update.details": "SEE DETAILS",
        "footer.author": "Ported by OKJI (Okladnoj)",
        "steam.status.ready": "READY",
        "steam.status.installingSteamCMD": "INSTALLING STEAMCMD...",
        "steam.status.awaitingCreds": "AWAITING CREDENTIALS",
        "steam.status.authenticating": "AUTHENTICATING...",
        "steam.status.steamGuard": "STEAM GUARD CODE REQUIRED",
        "steam.status.downloading": "DOWNLOADING ASSETS... %@",
        "steam.status.validating": "VALIDATING FILES...",
        "steam.status.downloadingPatch": "DOWNLOADING PATCH... %.0f%%",
        "steam.status.unpacking": "UNPACKING PATCH...",
        "steam.status.assetsReady": "ASSETS READY",
        "steam.status.error": "ERROR: %@",
        "patch.status.idle": "AWAITING ACTION",
        "patch.status.cleaning": "CLEANING CONFLICTING FILES...",
        "patch.status.downloading": "DOWNLOADING PATCH... %.0f%%",
        "patch.status.unpacking": "UNPACKING PATCH...",
        "patch.status.completed": "PATCH APPLIED SUCCESSFULLY",
        "patch.status.error": "ERROR: %@",
        "folder.prompt": "Select the Windows Game Folder (containing .big files)"
    ]

    static let ru: [String: String] = [
        "app.title": "GENERALS ONLINE",
        "app.subtitle": "MAC ПОРТ ОТ СООБЩЕСТВА",
        "tab.steam": "STEAM (РЕКОМЕНДУЕТСЯ)",
        "tab.local": "ЛОКАЛЬНЫЙ АРХИВ",
        "steam.credentials": "УЧЁТНЫЕ ДАННЫЕ STEAM",
        "steam.submit": "ВОЙТИ",
        "steam.cancel": "ОТМЕНА",
        "steam.download": "ЗАГРУЗИТЬ РЕСУРСЫ",
        "local.path": "ПУТЬ К ДАННЫМ ИГРЫ:",
        "local.locate": "ВЫБРАТЬ",
        "local.selectHint": "УКАЖИТЕ РОДИТЕЛЬСКУЮ ПАПКУ С ОБЕИМИ ВЕРСИЯМИ ИГРЫ",
        "local.invalidTarget": "НЕВЕРНАЯ ЦЕЛЬ — ini.big / inizh.big не найдены в подкаталогах",
        "action.launch": "ЗАПУСК",
        "action.patch": "ПАТЧ",
        "action.init": "ИНИЦИАЛИЗАЦИЯ...",
        "alert.launchError": "Ошибка запуска",
        "alert.ok": "OK",
        "alert.gameNotFound": "Игра не найдена",
        "alert.gameNotFoundMsg": "Аккаунт «%@» не владеет Command & Conquer™ Generals — Zero Hour.\n\nКупите игру в Steam и нажмите «Загрузить ресурсы» ещё раз.",
        "alert.openSteamStore": "Открыть Steam",
        "alert.close": "Закрыть",
        "alert.patchTitle": "Применить Community Patch?",
        "alert.patchMsg": "Ваши файлы игры могут быть необратимо изменены или удалены.\nЭто необходимо для онлайн-игры.",
        "alert.patchButton": "Патч",
        "update.available": "ДОСТУПНО ОБНОВЛЕНИЕ: v%@",
        "update.download": "СКАЧАТЬ ОБНОВЛЕНИЕ",
        "update.details": "ПОДРОБНЕЕ",
        "footer.author": "Портировал OKJI (Okladnoj)",
        "steam.status.ready": "ГОТОВО",
        "steam.status.installingSteamCMD": "УСТАНОВКА STEAMCMD...",
        "steam.status.awaitingCreds": "ОЖИДАНИЕ УЧЁТНЫХ ДАННЫХ",
        "steam.status.authenticating": "АУТЕНТИФИКАЦИЯ...",
        "steam.status.steamGuard": "ТРЕБУЕТСЯ КОД STEAM GUARD",
        "steam.status.downloading": "ЗАГРУЗКА РЕСУРСОВ... %@",
        "steam.status.validating": "ПРОВЕРКА ФАЙЛОВ...",
        "steam.status.downloadingPatch": "ЗАГРУЗКА ПАТЧА... %.0f%%",
        "steam.status.unpacking": "РАСПАКОВКА ПАТЧА...",
        "steam.status.assetsReady": "РЕСУРСЫ ГОТОВЫ",
        "steam.status.error": "ОШИБКА: %@",
        "patch.status.idle": "ОЖИДАНИЕ ДЕЙСТВИЯ",
        "patch.status.cleaning": "ОЧИСТКА КОНФЛИКТУЮЩИХ ФАЙЛОВ...",
        "patch.status.downloading": "ЗАГРУЗКА ПАТЧА... %.0f%%",
        "patch.status.unpacking": "РАСПАКОВКА ПАТЧА...",
        "patch.status.completed": "ПАТЧ УСПЕШНО ПРИМЕНЁН",
        "patch.status.error": "ОШИБКА: %@",
        "folder.prompt": "Выберите папку с Windows-версией игры (содержащую .big файлы)"
    ]

    static let uk: [String: String] = [
        "app.title": "GENERALS ONLINE",
        "app.subtitle": "MAC ПОРТ ВІД СПІЛЬНОТИ",
        "tab.steam": "STEAM (РЕКОМЕНДОВАНО)",
        "tab.local": "ЛОКАЛЬНИЙ АРХІВ",
        "steam.credentials": "ОБЛІКОВІ ДАНІ STEAM",
        "steam.submit": "УВІЙТИ",
        "steam.cancel": "СКАСУВАТИ",
        "steam.download": "ЗАВАНТАЖИТИ РЕСУРСИ",
        "local.path": "ШЛЯХ ДО ДАНИХ ГРИ:",
        "local.locate": "ВИБРАТИ",
        "local.selectHint": "ВКАЖІТЬ БАТЬКІВСЬКУ ПАПКУ З ОБОМА ВЕРСІЯМИ ГРИ",
        "local.invalidTarget": "НЕПРАВИЛЬНА ЦІЛЬ — ini.big / inizh.big не знайдено в підкаталогах",
        "action.launch": "ЗАПУСК",
        "action.patch": "ПАТЧ",
        "action.init": "ІНІЦІАЛІЗАЦІЯ...",
        "alert.launchError": "Помилка запуску",
        "alert.ok": "OK",
        "alert.gameNotFound": "Гру не знайдено",
        "alert.gameNotFoundMsg": "Акаунт «%@» не володіє Command & Conquer™ Generals — Zero Hour.\n\nПридбайте гру в Steam та натисніть «Завантажити ресурси» ще раз.",
        "alert.openSteamStore": "Відкрити Steam",
        "alert.close": "Закрити",
        "alert.patchTitle": "Застосувати Community Patch?",
        "alert.patchMsg": "Ваші файли гри можуть бути безповоротно змінені або видалені.\nЦе необхідно для онлайн-гри.",
        "alert.patchButton": "Патч",
        "update.available": "ДОСТУПНЕ ОНОВЛЕННЯ: v%@",
        "update.download": "ЗАВАНТАЖИТИ ОНОВЛЕННЯ",
        "update.details": "ДЕТАЛЬНІШЕ",
        "footer.author": "Портував OKJI (Okladnoj)",
        "steam.status.ready": "ГОТОВО",
        "steam.status.installingSteamCMD": "ВСТАНОВЛЕННЯ STEAMCMD...",
        "steam.status.awaitingCreds": "ОЧІКУВАННЯ ОБЛІКОВИХ ДАНИХ",
        "steam.status.authenticating": "АУТЕНТИФІКАЦІЯ...",
        "steam.status.steamGuard": "ПОТРІБЕН КОД STEAM GUARD",
        "steam.status.downloading": "ЗАВАНТАЖЕННЯ РЕСУРСІВ... %@",
        "steam.status.validating": "ПЕРЕВІРКА ФАЙЛІВ...",
        "steam.status.downloadingPatch": "ЗАВАНТАЖЕННЯ ПАТЧА... %.0f%%",
        "steam.status.unpacking": "РОЗПАКУВАННЯ ПАТЧА...",
        "steam.status.assetsReady": "РЕСУРСИ ГОТОВІ",
        "steam.status.error": "ПОМИЛКА: %@",
        "patch.status.idle": "ОЧІКУВАННЯ ДІЇ",
        "patch.status.cleaning": "ОЧИЩЕННЯ КОНФЛІКТНИХ ФАЙЛІВ...",
        "patch.status.downloading": "ЗАВАНТАЖЕННЯ ПАТЧА... %.0f%%",
        "patch.status.unpacking": "РОЗПАКУВАННЯ ПАТЧА...",
        "patch.status.completed": "ПАТЧ УСПІШНО ЗАСТОСОВАНО",
        "patch.status.error": "ПОМИЛКА: %@",
        "folder.prompt": "Виберіть папку з Windows-версією гри (що містить .big файли)"
    ]

    static let hi: [String: String] = [
        "app.title": "GENERALS ONLINE",
        "app.subtitle": "कम्युनिटी MAC पोर्ट",
        "tab.steam": "STEAM (अनुशंसित)",
        "tab.local": "लोकल आर्काइव",
        "steam.credentials": "STEAM क्रेडेंशियल",
        "steam.submit": "सबमिट",
        "steam.cancel": "रद्द करें",
        "steam.download": "एसेट्स डाउनलोड करें",
        "local.path": "गेम डेटा पथ:",
        "local.locate": "चुनें",
        "local.selectHint": "दोनों गेम संस्करणों वाली मूल डायरेक्टरी चुनें",
        "local.invalidTarget": "अमान्य लक्ष्य — सब-डायरेक्टरी में ini.big / inizh.big नहीं मिला",
        "action.launch": "लॉन्च",
        "action.patch": "पैच",
        "action.init": "शुरू हो रहा है...",
        "alert.launchError": "लॉन्च त्रुटि",
        "alert.ok": "OK",
        "alert.gameNotFound": "गेम नहीं मिला",
        "alert.gameNotFoundMsg": "अकाउंट \"%@\" के पास Command & Conquer™ Generals — Zero Hour नहीं है।\n\nSteam पर गेम खरीदें, फिर \"एसेट्स डाउनलोड करें\" दोबारा दबाएं।",
        "alert.openSteamStore": "Steam स्टोर खोलें",
        "alert.close": "बंद करें",
        "alert.patchTitle": "Community Patch लागू करें?",
        "alert.patchMsg": "आपकी गेम फ़ाइलें अपरिवर्तनीय रूप से संशोधित या हटाई जा सकती हैं।\nऑनलाइन खेलने के लिए यह आवश्यक है।",
        "alert.patchButton": "पैच",
        "update.available": "अपडेट उपलब्ध: v%@",
        "update.download": "अपडेट डाउनलोड करें",
        "update.details": "विवरण देखें",
        "footer.author": "पोर्ट: OKJI (Okladnoj)",
        "steam.status.ready": "तैयार",
        "steam.status.installingSteamCMD": "STEAMCMD इंस्टॉल हो रहा है...",
        "steam.status.awaitingCreds": "क्रेडेंशियल की प्रतीक्षा",
        "steam.status.authenticating": "प्रमाणीकरण...",
        "steam.status.steamGuard": "STEAM GUARD कोड आवश्यक",
        "steam.status.downloading": "एसेट्स डाउनलोड हो रहे हैं... %@",
        "steam.status.validating": "फ़ाइलें सत्यापित हो रही हैं...",
        "steam.status.downloadingPatch": "पैच डाउनलोड हो रहा है... %.0f%%",
        "steam.status.unpacking": "पैच अनपैक हो रहा है...",
        "steam.status.assetsReady": "एसेट्स तैयार",
        "steam.status.error": "त्रुटि: %@",
        "patch.status.idle": "कार्रवाई की प्रतीक्षा",
        "patch.status.cleaning": "विरोधी फ़ाइलें हटाई जा रही हैं...",
        "patch.status.downloading": "पैच डाउनलोड हो रहा है... %.0f%%",
        "patch.status.unpacking": "पैच अनपैक हो रहा है...",
        "patch.status.completed": "पैच सफलतापूर्वक लागू",
        "patch.status.error": "त्रुटि: %@",
        "folder.prompt": "Windows गेम फ़ोल्डर चुनें (.big फ़ाइलें वाला)"
    ]

    static let zh: [String: String] = [
        "app.title": "GENERALS ONLINE",
        "app.subtitle": "社区 MAC 移植版",
        "tab.steam": "STEAM（推荐）",
        "tab.local": "本地存档",
        "steam.credentials": "STEAM 凭据",
        "steam.submit": "提交",
        "steam.cancel": "取消",
        "steam.download": "下载资源",
        "local.path": "游戏数据路径：",
        "local.locate": "选择",
        "local.selectHint": "选择包含两个游戏版本的父目录",
        "local.invalidTarget": "无效目标 — 子目录中未检测到 ini.big / inizh.big",
        "action.launch": "启动",
        "action.patch": "修补",
        "action.init": "初始化中...",
        "alert.launchError": "启动错误",
        "alert.ok": "确定",
        "alert.gameNotFound": "未找到游戏",
        "alert.gameNotFoundMsg": "账户 \"%@\" 未拥有 Command & Conquer™ Generals — Zero Hour。\n\n请在 Steam 购买游戏，然后再次点击\"下载资源\"。",
        "alert.openSteamStore": "打开 Steam 商店",
        "alert.close": "关闭",
        "alert.patchTitle": "应用社区补丁？",
        "alert.patchMsg": "您的游戏文件可能会被不可逆地修改或删除。\n这是在线游戏所必需的。",
        "alert.patchButton": "修补",
        "update.available": "可用更新：v%@",
        "update.download": "下载更新",
        "update.details": "查看详情",
        "footer.author": "移植：OKJI (Okladnoj)",
        "steam.status.ready": "就绪",
        "steam.status.installingSteamCMD": "正在安装 STEAMCMD...",
        "steam.status.awaitingCreds": "等待凭据",
        "steam.status.authenticating": "认证中...",
        "steam.status.steamGuard": "需要 STEAM GUARD 代码",
        "steam.status.downloading": "正在下载资源... %@",
        "steam.status.validating": "正在验证文件...",
        "steam.status.downloadingPatch": "正在下载补丁... %.0f%%",
        "steam.status.unpacking": "正在解压补丁...",
        "steam.status.assetsReady": "资源就绪",
        "steam.status.error": "错误：%@",
        "patch.status.idle": "等待操作",
        "patch.status.cleaning": "正在清理冲突文件...",
        "patch.status.downloading": "正在下载补丁... %.0f%%",
        "patch.status.unpacking": "正在解压补丁...",
        "patch.status.completed": "补丁应用成功",
        "patch.status.error": "错误：%@",
        "folder.prompt": "选择 Windows 游戏文件夹（包含 .big 文件）"
    ]

    static let ar: [String: String] = [
        "app.title": "GENERALS ONLINE",
        "app.subtitle": "نسخة MAC من المجتمع",
        "tab.steam": "STEAM (موصى به)",
        "tab.local": "أرشيف محلي",
        "steam.credentials": "بيانات STEAM",
        "steam.submit": "إرسال",
        "steam.cancel": "إلغاء",
        "steam.download": "تحميل الموارد",
        "local.path": "مسار بيانات اللعبة:",
        "local.locate": "تحديد",
        "local.selectHint": "حدد المجلد الرئيسي الذي يحتوي على نسختي اللعبة",
        "local.invalidTarget": "هدف غير صالح — لم يتم العثور على ini.big / inizh.big في المجلدات الفرعية",
        "action.launch": "تشغيل",
        "action.patch": "تصحيح",
        "action.init": "جارٍ التهيئة...",
        "alert.launchError": "خطأ في التشغيل",
        "alert.ok": "موافق",
        "alert.gameNotFound": "اللعبة غير موجودة",
        "alert.gameNotFoundMsg": "الحساب \"%@\" لا يملك Command & Conquer™ Generals — Zero Hour.\n\nقم بشراء اللعبة على Steam، ثم اضغط \"تحميل الموارد\" مرة أخرى.",
        "alert.openSteamStore": "فتح متجر Steam",
        "alert.close": "إغلاق",
        "alert.patchTitle": "تطبيق تصحيح المجتمع؟",
        "alert.patchMsg": "قد يتم تعديل ملفات اللعبة أو حذفها بشكل لا رجعة فيه.\nهذا مطلوب للعب عبر الإنترنت.",
        "alert.patchButton": "تصحيح",
        "update.available": "تحديث متاح: v%@",
        "update.download": "تحميل التحديث",
        "update.details": "عرض التفاصيل",
        "footer.author": "نقل: OKJI (Okladnoj)",
        "steam.status.ready": "جاهز",
        "steam.status.installingSteamCMD": "جارٍ تثبيت STEAMCMD...",
        "steam.status.awaitingCreds": "في انتظار البيانات",
        "steam.status.authenticating": "جارٍ المصادقة...",
        "steam.status.steamGuard": "مطلوب رمز STEAM GUARD",
        "steam.status.downloading": "جارٍ تحميل الموارد... %@",
        "steam.status.validating": "جارٍ التحقق من الملفات...",
        "steam.status.downloadingPatch": "جارٍ تحميل التصحيح... %.0f%%",
        "steam.status.unpacking": "جارٍ فك التصحيح...",
        "steam.status.assetsReady": "الموارد جاهزة",
        "steam.status.error": "خطأ: %@",
        "patch.status.idle": "في انتظار الإجراء",
        "patch.status.cleaning": "جارٍ تنظيف الملفات المتعارضة...",
        "patch.status.downloading": "جارٍ تحميل التصحيح... %.0f%%",
        "patch.status.unpacking": "جارٍ فك التصحيح...",
        "patch.status.completed": "تم تطبيق التصحيح بنجاح",
        "patch.status.error": "خطأ: %@",
        "folder.prompt": "حدد مجلد لعبة Windows (يحتوي على ملفات .big)"
    ]

    static let kk: [String: String] = [
        "app.title": "GENERALS ONLINE",
        "app.subtitle": "ҚАУЫМДАСТЫҚ MAC ПОРТЫ",
        "tab.steam": "STEAM (ҰСЫНЫЛАДЫ)",
        "tab.local": "ЖЕРГІЛІКТІ МҰРАҒАТ",
        "steam.credentials": "STEAM ДЕРЕКТЕРІ",
        "steam.submit": "ЖІБЕРУ",
        "steam.cancel": "БОЛДЫРМАУ",
        "steam.download": "РЕСУРСТАРДЫ ЖҮКТЕУ",
        "local.path": "ОЙЫН ДЕРЕКТЕРІНІҢ ЖОЛЫ:",
        "local.locate": "ТАҢДАУ",
        "local.selectHint": "ОЙЫННЫҢ ЕКІ НҰСҚАСЫ БАР АТА-АНА ҚАЛТАСЫН КӨРСЕТІҢІЗ",
        "local.invalidTarget": "ЖАРАМСЫЗ МАҚСАТ — ішкі қалталарда ini.big / inizh.big табылмады",
        "action.launch": "ІСКЕ ҚОСУ",
        "action.patch": "ПАТЧ",
        "action.init": "ИНИЦИАЛИЗАЦИЯ...",
        "alert.launchError": "Іске қосу қатесі",
        "alert.ok": "OK",
        "alert.gameNotFound": "Ойын табылмады",
        "alert.gameNotFoundMsg": "«%@» тіркелгісінде Command & Conquer™ Generals — Zero Hour жоқ.\n\nSteam-де ойынды сатып алыңыз, содан кейін «Ресурстарды жүктеу» түймесін қайта басыңыз.",
        "alert.openSteamStore": "Steam дүкенін ашу",
        "alert.close": "Жабу",
        "alert.patchTitle": "Community Patch қолдану керек пе?",
        "alert.patchMsg": "Ойын файлдарыңыз қайтарылмайтын түрде өзгертілуі немесе жойылуы мүмкін.\nБұл онлайн ойнау үшін қажет.",
        "alert.patchButton": "Патч",
        "update.available": "ЖАҢАРТУ ҚОЛЖЕТІМДІ: v%@",
        "update.download": "ЖАҢАРТУДЫ ЖҮКТЕУ",
        "update.details": "ТОЛЫҒЫРАҚ",
        "footer.author": "Порт: OKJI (Okladnoj)",
        "steam.status.ready": "ДАЙЫН",
        "steam.status.installingSteamCMD": "STEAMCMD ОРНАТЫЛУДА...",
        "steam.status.awaitingCreds": "ДЕРЕКТЕРДІ КҮТУ",
        "steam.status.authenticating": "АУТЕНТИФИКАЦИЯ...",
        "steam.status.steamGuard": "STEAM GUARD КОДЫ ҚАЖЕТ",
        "steam.status.downloading": "РЕСУРСТАР ЖҮКТЕЛУДЕ... %@",
        "steam.status.validating": "ФАЙЛДАР ТЕКСЕРІЛУДЕ...",
        "steam.status.downloadingPatch": "ПАТЧ ЖҮКТЕЛУДЕ... %.0f%%",
        "steam.status.unpacking": "ПАТЧ АШЫЛУДА...",
        "steam.status.assetsReady": "РЕСУРСТАР ДАЙЫН",
        "steam.status.error": "ҚАТЕ: %@",
        "patch.status.idle": "ӘРЕКЕТТІ КҮТУ",
        "patch.status.cleaning": "ҚАЙШЫЛЫҚТЫ ФАЙЛДАР ТАЗАЛАНУДА...",
        "patch.status.downloading": "ПАТЧ ЖҮКТЕЛУДЕ... %.0f%%",
        "patch.status.unpacking": "ПАТЧ АШЫЛУДА...",
        "patch.status.completed": "ПАТЧ СӘТТІ ҚОЛДАНЫЛДЫ",
        "patch.status.error": "ҚАТЕ: %@",
        "folder.prompt": "Windows ойын қалтасын таңдаңыз (.big файлдары бар)"
    ]

    static let vi: [String: String] = [
        "app.title": "GENERALS ONLINE",
        "app.subtitle": "PHIÊN BẢN MAC CỘNG ĐỒNG",
        "tab.steam": "STEAM (KHUYẾN NGHỊ)",
        "tab.local": "LƯU TRỮ CỤC BỘ",
        "steam.credentials": "THÔNG TIN STEAM",
        "steam.submit": "GỬI",
        "steam.cancel": "HỦY",
        "steam.download": "TẢI TÀI NGUYÊN",
        "local.path": "ĐƯỜNG DẪN DỮ LIỆU TRÒ CHƠI:",
        "local.locate": "CHỌN",
        "local.selectHint": "CHỌN THƯ MỤC GỐC CHỨA CẢ HAI PHIÊN BẢN TRÒ CHƠI",
        "local.invalidTarget": "MỤC TIÊU KHÔNG HỢP LỆ — Không tìm thấy ini.big / inizh.big trong thư mục con",
        "action.launch": "CHẠY",
        "action.patch": "VÁ",
        "action.init": "ĐANG KHỞI TẠO...",
        "alert.launchError": "Lỗi khởi chạy",
        "alert.ok": "OK",
        "alert.gameNotFound": "Không tìm thấy trò chơi",
        "alert.gameNotFoundMsg": "Tài khoản \"%@\" không sở hữu Command & Conquer™ Generals — Zero Hour.\n\nMua trò chơi trên Steam, sau đó nhấn \"Tải tài nguyên\" lần nữa.",
        "alert.openSteamStore": "Mở Steam Store",
        "alert.close": "Đóng",
        "alert.patchTitle": "Áp dụng Community Patch?",
        "alert.patchMsg": "Các tệp trò chơi có thể bị sửa đổi hoặc xóa không thể khôi phục.\nĐiều này cần thiết để chơi trực tuyến.",
        "alert.patchButton": "Vá",
        "update.available": "CÓ BẢN CẬP NHẬT: v%@",
        "update.download": "TẢI BẢN CẬP NHẬT",
        "update.details": "XEM CHI TIẾT",
        "footer.author": "Chuyển đổi: OKJI (Okladnoj)",
        "steam.status.ready": "SẴN SÀNG",
        "steam.status.installingSteamCMD": "ĐANG CÀI ĐẶT STEAMCMD...",
        "steam.status.awaitingCreds": "ĐANG CHỜ THÔNG TIN",
        "steam.status.authenticating": "ĐANG XÁC THỰC...",
        "steam.status.steamGuard": "CẦN MÃ STEAM GUARD",
        "steam.status.downloading": "ĐANG TẢI TÀI NGUYÊN... %@",
        "steam.status.validating": "ĐANG KIỂM TRA TỆP...",
        "steam.status.downloadingPatch": "ĐANG TẢI BẢN VÁ... %.0f%%",
        "steam.status.unpacking": "ĐANG GIẢI NÉN BẢN VÁ...",
        "steam.status.assetsReady": "TÀI NGUYÊN SẴN SÀNG",
        "steam.status.error": "LỖI: %@",
        "patch.status.idle": "CHỜ HÀNH ĐỘNG",
        "patch.status.cleaning": "ĐANG XÓA TỆP XUNG ĐỘT...",
        "patch.status.downloading": "ĐANG TẢI BẢN VÁ... %.0f%%",
        "patch.status.unpacking": "ĐANG GIẢI NÉN BẢN VÁ...",
        "patch.status.completed": "ÁP DỤNG BẢN VÁ THÀNH CÔNG",
        "patch.status.error": "LỖI: %@",
        "folder.prompt": "Chọn thư mục trò chơi Windows (chứa tệp .big)"
    ]

    static let pl: [String: String] = [
        "app.title": "GENERALS ONLINE",
        "app.subtitle": "PORT MAC OD SPOŁECZNOŚCI",
        "tab.steam": "STEAM (ZALECANE)",
        "tab.local": "ARCHIWUM LOKALNE",
        "steam.credentials": "DANE LOGOWANIA STEAM",
        "steam.submit": "ZALOGUJ",
        "steam.cancel": "ANULUJ",
        "steam.download": "POBIERZ ZASOBY",
        "local.path": "ŚCIEŻKA DANYCH GRY:",
        "local.locate": "WYBIERZ",
        "local.selectHint": "WSKAŻ FOLDER NADRZĘDNY ZAWIERAJĄCY OBE WERSJE GRY",
        "local.invalidTarget": "NIEPRAWIDŁOWY CEL — Nie wykryto ini.big / inizh.big w podkatalogach",
        "action.launch": "URUCHOM",
        "action.patch": "ŁATKA",
        "action.init": "INICJALIZACJA...",
        "alert.launchError": "Błąd uruchamiania",
        "alert.ok": "OK",
        "alert.gameNotFound": "Nie znaleziono gry",
        "alert.gameNotFoundMsg": "Konto \"%@\" nie posiada Command & Conquer™ Generals — Zero Hour.\n\nKup grę na Steam, a następnie ponownie naciśnij \"Pobierz zasoby\".",
        "alert.openSteamStore": "Otwórz Steam Store",
        "alert.close": "Zamknij",
        "alert.patchTitle": "Zastosować Community Patch?",
        "alert.patchMsg": "Pliki gry mogą zostać nieodwracalnie zmodyfikowane lub usunięte.\nJest to wymagane do gry online.",
        "alert.patchButton": "Łatka",
        "update.available": "DOSTĘPNA AKTUALIZACJA: v%@",
        "update.download": "POBIERZ AKTUALIZACJĘ",
        "update.details": "SZCZEGÓŁY",
        "footer.author": "Port: OKJI (Okladnoj)",
        "steam.status.ready": "GOTOWE",
        "steam.status.installingSteamCMD": "INSTALOWANIE STEAMCMD...",
        "steam.status.awaitingCreds": "OCZEKIWANIE NA DANE",
        "steam.status.authenticating": "UWIERZYTELNIANIE...",
        "steam.status.steamGuard": "WYMAGANY KOD STEAM GUARD",
        "steam.status.downloading": "POBIERANIE ZASOBÓW... %@",
        "steam.status.validating": "WERYFIKACJA PLIKÓW...",
        "steam.status.downloadingPatch": "POBIERANIE ŁATKI... %.0f%%",
        "steam.status.unpacking": "ROZPAKOWYWANIE ŁATKI...",
        "steam.status.assetsReady": "ZASOBY GOTOWE",
        "steam.status.error": "BŁĄD: %@",
        "patch.status.idle": "OCZEKIWANIE NA DZIAŁANIE",
        "patch.status.cleaning": "CZYSZCZENIE PLIKÓW KONFLIKTOWYCH...",
        "patch.status.downloading": "POBIERANIE ŁATKI... %.0f%%",
        "patch.status.unpacking": "ROZPAKOWYWANIE ŁATKI...",
        "patch.status.completed": "ŁATKA ZASTOSOWANA POMYŚLNIE",
        "patch.status.error": "BŁĄD: %@",
        "folder.prompt": "Wybierz folder gry Windows (zawierający pliki .big)"
    ]

    static let de: [String: String] = [
        "app.title": "GENERALS ONLINE",
        "app.subtitle": "COMMUNITY MAC PORT",
        "tab.steam": "STEAM (EMPFOHLEN)",
        "tab.local": "LOKALES ARCHIV",
        "steam.credentials": "STEAM-ANMELDEDATEN",
        "steam.submit": "ANMELDEN",
        "steam.cancel": "ABBRECHEN",
        "steam.download": "ASSETS HERUNTERLADEN",
        "local.path": "SPIELDATEN-PFAD:",
        "local.locate": "AUSWÄHLEN",
        "local.selectHint": "WÄHLEN SIE DEN ÜBERGEORDNETEN ORDNER MIT BEIDEN SPIELVERSIONEN",
        "local.invalidTarget": "UNGÜLTIGES ZIEL — Keine ini.big / inizh.big in Unterverzeichnissen gefunden",
        "action.launch": "STARTEN",
        "action.patch": "PATCH",
        "action.init": "INITIALISIERUNG...",
        "alert.launchError": "Startfehler",
        "alert.ok": "OK",
        "alert.gameNotFound": "Spiel nicht gefunden",
        "alert.gameNotFoundMsg": "Das Konto \"%@\" besitzt Command & Conquer™ Generals — Zero Hour nicht.\n\nKaufen Sie das Spiel auf Steam und klicken Sie erneut auf \"Assets herunterladen\".",
        "alert.openSteamStore": "Steam Store öffnen",
        "alert.close": "Schließen",
        "alert.patchTitle": "Community Patch anwenden?",
        "alert.patchMsg": "Ihre Spieldateien können unwiderruflich verändert oder gelöscht werden.\nDies ist für Online-Spiele erforderlich.",
        "alert.patchButton": "Patch",
        "update.available": "UPDATE VERFÜGBAR: v%@",
        "update.download": "UPDATE HERUNTERLADEN",
        "update.details": "DETAILS ANZEIGEN",
        "footer.author": "Portiert von OKJI (Okladnoj)",
        "steam.status.ready": "BEREIT",
        "steam.status.installingSteamCMD": "STEAMCMD WIRD INSTALLIERT...",
        "steam.status.awaitingCreds": "WARTE AUF ANMELDEDATEN",
        "steam.status.authenticating": "AUTHENTIFIZIERUNG...",
        "steam.status.steamGuard": "STEAM GUARD CODE ERFORDERLICH",
        "steam.status.downloading": "ASSETS WERDEN HERUNTERGELADEN... %@",
        "steam.status.validating": "DATEIEN WERDEN ÜBERPRÜFT...",
        "steam.status.downloadingPatch": "PATCH WIRD HERUNTERGELADEN... %.0f%%",
        "steam.status.unpacking": "PATCH WIRD ENTPACKT...",
        "steam.status.assetsReady": "ASSETS BEREIT",
        "steam.status.error": "FEHLER: %@",
        "patch.status.idle": "WARTE AUF AKTION",
        "patch.status.cleaning": "KONFLIKTDATEIEN WERDEN BEREINIGT...",
        "patch.status.downloading": "PATCH WIRD HERUNTERGELADEN... %.0f%%",
        "patch.status.unpacking": "PATCH WIRD ENTPACKT...",
        "patch.status.completed": "PATCH ERFOLGREICH ANGEWENDET",
        "patch.status.error": "FEHLER: %@",
        "folder.prompt": "Windows-Spielordner auswählen (mit .big-Dateien)"
    ]

    static let es: [String: String] = [
        "app.title": "GENERALS ONLINE",
        "app.subtitle": "PORT MAC DE LA COMUNIDAD",
        "tab.steam": "STEAM (RECOMENDADO)",
        "tab.local": "ARCHIVO LOCAL",
        "steam.credentials": "CREDENCIALES DE STEAM",
        "steam.submit": "ENVIAR",
        "steam.cancel": "CANCELAR",
        "steam.download": "DESCARGAR RECURSOS",
        "local.path": "RUTA DE DATOS DEL JUEGO:",
        "local.locate": "SELECCIONAR",
        "local.selectHint": "SELECCIONE LA CARPETA PRINCIPAL QUE CONTIENE AMBAS VERSIONES DEL JUEGO",
        "local.invalidTarget": "OBJETIVO INVÁLIDO — No se detectó ini.big / inizh.big en subdirectorios",
        "action.launch": "INICIAR",
        "action.patch": "PARCHE",
        "action.init": "INICIALIZANDO...",
        "alert.launchError": "Error de inicio",
        "alert.ok": "OK",
        "alert.gameNotFound": "Juego no encontrado",
        "alert.gameNotFoundMsg": "La cuenta \"%@\" no posee Command & Conquer™ Generals — Zero Hour.\n\nCompra el juego en Steam y luego presiona \"Descargar recursos\" nuevamente.",
        "alert.openSteamStore": "Abrir Steam Store",
        "alert.close": "Cerrar",
        "alert.patchTitle": "¿Aplicar Community Patch?",
        "alert.patchMsg": "Los archivos del juego pueden ser modificados o eliminados de forma irreversible.\nEsto es necesario para jugar en línea.",
        "alert.patchButton": "Parche",
        "update.available": "ACTUALIZACIÓN DISPONIBLE: v%@",
        "update.download": "DESCARGAR ACTUALIZACIÓN",
        "update.details": "VER DETALLES",
        "footer.author": "Portado por OKJI (Okladnoj)",
        "steam.status.ready": "LISTO",
        "steam.status.installingSteamCMD": "INSTALANDO STEAMCMD...",
        "steam.status.awaitingCreds": "ESPERANDO CREDENCIALES",
        "steam.status.authenticating": "AUTENTICANDO...",
        "steam.status.steamGuard": "CÓDIGO STEAM GUARD REQUERIDO",
        "steam.status.downloading": "DESCARGANDO RECURSOS... %@",
        "steam.status.validating": "VERIFICANDO ARCHIVOS...",
        "steam.status.downloadingPatch": "DESCARGANDO PARCHE... %.0f%%",
        "steam.status.unpacking": "DESCOMPRIMIENDO PARCHE...",
        "steam.status.assetsReady": "RECURSOS LISTOS",
        "steam.status.error": "ERROR: %@",
        "patch.status.idle": "ESPERANDO ACCIÓN",
        "patch.status.cleaning": "LIMPIANDO ARCHIVOS EN CONFLICTO...",
        "patch.status.downloading": "DESCARGANDO PARCHE... %.0f%%",
        "patch.status.unpacking": "DESCOMPRIMIENDO PARCHE...",
        "patch.status.completed": "PARCHE APLICADO CON ÉXITO",
        "patch.status.error": "ERROR: %@",
        "folder.prompt": "Seleccione la carpeta del juego de Windows (que contiene archivos .big)"
    ]

    static let tr: [String: String] = [
        "app.title": "GENERALS ONLINE",
        "app.subtitle": "TOPLULUK MAC PORTU",
        "tab.steam": "STEAM (ÖNERİLEN)",
        "tab.local": "YEREL ARŞİV",
        "steam.credentials": "STEAM KİMLİK BİLGİLERİ",
        "steam.submit": "GÖNDER",
        "steam.cancel": "İPTAL",
        "steam.download": "KAYNAKLARI İNDİR",
        "local.path": "OYUN VERİ YOLU:",
        "local.locate": "SEÇ",
        "local.selectHint": "HER İKİ OYUN SÜRÜMÜNÜ İÇEREN ANA KLASÖRÜ SEÇİN",
        "local.invalidTarget": "GEÇERSİZ HEDEF — Alt dizinlerde ini.big / inizh.big tespit edilemedi",
        "action.launch": "BAŞLAT",
        "action.patch": "YAMA",
        "action.init": "BAŞLATILIYOR...",
        "alert.launchError": "Başlatma Hatası",
        "alert.ok": "Tamam",
        "alert.gameNotFound": "Oyun Bulunamadı",
        "alert.gameNotFoundMsg": "\"%@\" hesabı Command & Conquer™ Generals — Zero Hour'a sahip değil.\n\nOyunu Steam'den satın alın, ardından tekrar \"Kaynakları İndir\" düğmesine basın.",
        "alert.openSteamStore": "Steam Mağazasını Aç",
        "alert.close": "Kapat",
        "alert.patchTitle": "Community Patch Uygulanacak mı?",
        "alert.patchMsg": "Oyun dosyalarınız geri dönüşü olmayacak şekilde değiştirilebilir veya silinebilir.\nÇevrimiçi oynamak için bu gereklidir.",
        "alert.patchButton": "Yama",
        "update.available": "GÜNCELLEME MEVCUT: v%@",
        "update.download": "GÜNCELLEMEYİ İNDİR",
        "update.details": "DETAYLARI GÖR",
        "footer.author": "Port: OKJI (Okladnoj)",
        "steam.status.ready": "HAZIR",
        "steam.status.installingSteamCMD": "STEAMCMD KURULUYOR...",
        "steam.status.awaitingCreds": "KİMLİK BİLGİLERİ BEKLENİYOR",
        "steam.status.authenticating": "KİMLİK DOĞRULAMA...",
        "steam.status.steamGuard": "STEAM GUARD KODU GEREKLİ",
        "steam.status.downloading": "KAYNAKLAR İNDİRİLİYOR... %@",
        "steam.status.validating": "DOSYALAR DOĞRULANIYOR...",
        "steam.status.downloadingPatch": "YAMA İNDİRİLİYOR... %.0f%%",
        "steam.status.unpacking": "YAMA AÇILIYOR...",
        "steam.status.assetsReady": "KAYNAKLAR HAZIR",
        "steam.status.error": "HATA: %@",
        "patch.status.idle": "EYLEM BEKLENİYOR",
        "patch.status.cleaning": "ÇAKIŞAN DOSYALAR TEMİZLENİYOR...",
        "patch.status.downloading": "YAMA İNDİRİLİYOR... %.0f%%",
        "patch.status.unpacking": "YAMA AÇILIYOR...",
        "patch.status.completed": "YAMA BAŞARIYLA UYGULANDI",
        "patch.status.error": "HATA: %@",
        "folder.prompt": "Windows oyun klasörünü seçin (.big dosyaları içeren)"
    ]
}
