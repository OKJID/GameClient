# Файловая система macOS порта (C&C Generals Zero Hour)

## Windows-флоу (Reference)

На Windows бинарник `generals.exe` лежит внутри папки Zero Hour. CWD при запуске = папка ZH.

```
C:\EA Games\
├── Command and Conquer Generals Zero Hour\
│   ├── generals.exe          ← CWD = здесь
│   ├── *.big                 ← ZH big-файлы
│   └── Data/Scripts/SkirmishScripts.scb
└── Command and Conquer Generals\          ← Registry InstallPath
    ├── *.big                 ← Base big-файлы
    └── Data/...
```

`Win32BIGFileSystem::init()`:
1. `loadBigFilesFromDirectory("", "*.big")` → CWD = ZH
2. `GetStringFromGeneralsRegistry("", "InstallPath")` → путь к Base из реестра
3. `loadBigFilesFromDirectory(installPath, "*.big")` → Base

**Приоритет (первый загруженный побеждает):** `Loose CWD > BIG CWD > BIG Base`

## macOS: зеркалирование Windows через `chdir()`

На macOS бинарник находится внутри `.app` бандла или в директории сборки — **не** в папке с данными игры. Чтобы shared-код ядра (`StdBIGFileSystem::init`) работал идентично Windows без единого `#ifdef`, мы воссоздаём Windows-окружение на старте.

### Переменная `GENERALS_INSTALL_PATH`

```bash
export GENERALS_INSTALL_PATH="/Users/okji/dev/games/Command and Conquer - Generals"
```

Указывает на **рутовую** директорию, содержащую подпапки с режимами игры.

### Структура данных

```
Command and Conquer - Generals/
├── Command and Conquer Generals Zero Hour/   ← ZH (маркер: INIZH.big)
│   ├── *.big
│   └── Data/Scripts/SkirmishScripts.scb
└── Command and Conquer Generals/             ← Base (маркер: INI.big, нет INIZH.big)
    ├── *.big
    └── Data/...
```

> [!IMPORTANT]
> Имена подпапок **не хардкодятся**. Детектор сканирует содержимое и ищет маркеры.

### Инициализация (MacOSGameEngine::init)

До вызова `GameEngine::init()`:

1. Читается `GENERALS_INSTALL_PATH`
2. `DetectGameModes(rootPath)` сканирует субдиректории:
   - **ZH** = содержит `INIZH.big`
   - **Base** = содержит `INI.big`, но НЕ `INIZH.big`
3. `chdir(zhPath)` — теперь CWD = ZH, как на Windows
4. `basePath` сохраняется для `registry.cpp`

После этого `GameEngine::init()` → `StdBIGFileSystem::init()` работает **идентично Windows**:
- `loadBigFilesFromDirectory("", "*.big")` → CWD = ZH ✅
- `GetStringFromGeneralsRegistry("", "InstallPath")` → `basePath` ✅
- `loadBigFilesFromDirectory(basePath, "*.big")` → Base ✅

## Общая архитектура

| Компонент | Класс | Назначение |
|-----------|-------|------------|
| `TheLocalFileSystem` | `MacOSLocalFileSystem` | Loose-файлы на диске + Slash Boundary + case-insensitive поиск |
| `TheArchiveFileSystem` | `StdBIGFileSystem` | Файлы внутри `.big` архивов |

При вызове `TheFileSystem->openFile(path)`:
1. Сначала ищется **loose-файл** через `TheLocalFileSystem->openFile`
2. Если не найден — ищется в **`.big` архивах** через `TheArchiveFileSystem->openFile`

> [!IMPORTANT]
> Loose-файлы **всегда** имеют приоритет над `.big` архивами.

## Slash Boundary (Граница нормализации)

Движок SAGE жестко завязан на Windows-пути (`\` и case-insensitive). `MacOSLocalFileSystem` изолирует это от POSIX:

**Inbound (в OS):** Методы `openFile`, `doesFileExist`, `getFileInfo` пробрасывают путь через `resolveWithSearchPaths`:
- `\` → `/`
- Case-insensitive сопоставление через `strcasecmp`

**Outbound (в движок SAGE):** `getFileListInDirectory` конвертирует `/` → `\` в возвращаемых путях.

> [!WARNING]
> Никаких `#ifdef __APPLE__` внутри `StdLocalFileSystem.cpp`, `MapUtil.cpp`, `INIMapCache.cpp`.
> Shared-код должен работать так, будто он на Windows.

## addSearchPath (macOS-специфика)

`MacOSLocalFileSystem` имеет `addSearchPath` / `resolveWithSearchPaths` — этого нет в `Win32LocalFileSystem`. Нужно потому что:
- На macOS файловая система может быть case-sensitive
- Loose-файлы ищутся сначала в CWD, затем в search paths (ZH, Base)

Регистрация search paths происходит в `MacOSLocalFileSystem::init()`.

### Приоритет loose-файлов

| # | Где ищет | Пример |
|---|---------|--------|
| 1 | CWD (= ZH после chdir) | `./Data/Scripts/foo.scb` |
| 2 | ZH path (search path) | `.../Zero Hour/Data/Scripts/foo.scb` |
| 3 | Base path (search path) | `.../Generals/Data/Scripts/foo.scb` |

### Итоговый приоритет при `openFile`

```
Loose CWD > Loose ZH > Loose Base > BIG CWD > BIG Base
```

## registry.cpp (`#ifdef __APPLE__`)

На Windows `GetStringFromGeneralsRegistry("", "InstallPath")` возвращает путь к Base из реестра.

На macOS `getStringFromRegistry` уже обёрнут `#ifdef __APPLE__`. Для ключа `InstallPath` возвращает `basePath`, обнаруженный детектором при старте.

## Дуальность Core / Platform

В проекте существуют **два** `StdLocalFileSystem`:

| Файл | Расположение |
|------|-------------|
| `Core/GameEngineDevice/.../StdLocalFileSystem.h/.cpp` | Core (базовый класс) |
| `Platform/MacOS/.../MacOSLocalFileSystem.h/.mm` | Platform (macOS, наследует StdLocalFileSystem) |

`MacOSGameEngine::createLocalFileSystem()` возвращает `NEW MacOSLocalFileSystem`.

> [!WARNING]
> Ранее factory возвращал `NEW StdLocalFileSystem` — это было ошибкой.
> `MacOSLocalFileSystem` наследует `StdLocalFileSystem` и переопределяет все методы
> с Slash Boundary и search path логикой.

## Нечувствительность к регистру в .big архивах (ArchiveFileSystem)

Windows-версия игры эмулирует case-insensitive подход, в то время как `std::multimap<AsciiString, ArchiveFile *>` (используемый для хранения дерева файлов) полагается на `AsciiString::operator<`, который является **строго регистрозависимым** (case-sensitive, использует `strcmp`).

Чтобы гарантировать Windows-совместимый поиск файлов внутри `.big` (крайне важно для `MapCache.ini`), реализован следующий механизм:
1. При сканировании содержимого `.big` архивов внутри `ArchiveFileSystem::loadIntoDirectoryTree`, извлечённые имена файлов **принудительно переводятся в нижний регистр** (`lowerToken.toLower()`), прежде чем сохраняться в ключи словаря `m_files`.
2. Удаление `.toLower()` на этом этапе критически ломает `find()` в Clang libc++, так как запросы часто приходят в другом регистре.
3. Порядок загрузки (Shadowing): `.big` файлы основной игры (Base) и аддона (ZH) используют параметр `overwrite = FALSE`, что вставляет дублирующиеся ключи в конец. `StdBIGFileSystem` загружает папки ZH, а затем Base, обеспечивая правильный порядок кэширования для ресурсов движка.
3. Отсутствие перевода в нижний регистр ломает базовый поиск (например, `doesFileExist("Maps\\MapCache.ini")`), что каскадно обрывает загрузку официальных карт.

## Фильтрация дубликата INIZH.big

В `loadBigFilesFromDirectory` пропускается `INIZH.big` из подпапки `Data/INI/`,
потому что многие цифровые версии содержат дубликат, что приводит к конфликту CRC.

## Критические loose-файлы

Эти файлы **не запакованы** в `.big` и ищутся через CWD / search paths:

| Файл | Зачем |
|------|-------|
| `Data/Scripts/SkirmishScripts.scb` | AI скрипты для скирмиша (строительство, атаки) |
| `Data/Scripts/MultiplayerScripts.scb` | Скрипты для мультиплеера |
| `Data/INI/*.ini` | Конфигурация юнитов, оружия, зданий |

> [!CAUTION]
> Если `SkirmishScripts.scb` не найден, бот в скирмише **не строит и не атакует**.
