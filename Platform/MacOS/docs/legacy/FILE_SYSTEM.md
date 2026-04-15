# Файловая система macOS порта (C&C Generals Zero Hour)

## Общая архитектура

Файловая система состоит из двух абстракций, работающих в связке:

| Компонент | Класс | Назначение |
|-----------|-------|------------|
| `TheLocalFileSystem` | `StdLocalFileSystem` | Loose-файлы на диске (INI, скрипты, текстуры) |
| `TheArchiveFileSystem` | `StdBIGFileSystem` | Файлы внутри `.big` архивов |

При вызове `TheFileSystem->openFile(path)`:
1. Сначала ищется **loose-файл** через `TheLocalFileSystem->openFile`
2. Если не найден — ищется в **`.big` архивах** через `TheArchiveFileSystem->openFile`

> [!IMPORTANT]
> Loose-файлы **всегда** имеют приоритет над `.big` архивами.

## Переменная окружения `GENERALS_INSTALL_PATH`

На Windows игра использует реестр (`InstallPath`). На macOS — переменную окружения.

```bash
# build_run_mac.sh
export GENERALS_INSTALL_PATH="/Users/okji/dev/games/Command and Conquer - Generals"
```

Ожидаемая структура внутри:
```
Command and Conquer - Generals/
├── Command and Conquer Generals Zero Hour/   ← ZH
│   ├── *.big
│   └── Data/Scripts/SkirmishScripts.scb
└── Command and Conquer Generals/             ← Base
    ├── *.big
    └── Data/...
```

Имена подпапок **захардкожены** в `StdBIGFileSystem::init()`.

## Инициализация (`StdBIGFileSystem::init`)

Порядок инициализации при запуске (macOS, `RTS_ZEROHOUR`):

### 1. Регистрация search paths для loose-файлов

```
TheLocalFileSystem->addSearchPath(zhPath)       ← первый
TheLocalFileSystem->addSearchPath(genBasePath)   ← второй
```

### 2. Загрузка `.big` архивов

```
loadBigFilesFromDirectory("", "*.big")           ← CWD (dev mods)
loadBigFilesFromDirectory(zhPath, "*.big")        ← Zero Hour
loadBigFilesFromDirectory(genBasePath, "*.big")   ← Base Game
```

## Приоритеты

### Loose-файлы (`resolveWithSearchPaths`)

| # | Где ищет | Пример |
|---|---------|--------|
| 1 | CWD (папка исходников) | `./Data/Scripts/foo.scb` |
| 2 | ZH path | `.../Zero Hour/Data/Scripts/foo.scb` |
| 3 | Base path | `.../Generals/Data/Scripts/foo.scb` |

### `.big` архивы (первый загруженный побеждает)

| # | Источник | Приоритет |
|---|---------|-----------|
| 1 | CWD `.big` | Наивысший (dev-моды, кастомный UI) |
| 2 | ZH `.big` | Средний |
| 3 | Base `.big` | Низший |

### Итоговый приоритет при `openFile`

```
Loose CWD > Loose ZH > Loose Base > BIG CWD > BIG ZH > BIG Base
```

## Дуальность Core / Platform

В проекте существуют **два** `StdLocalFileSystem`:

| Файл | Расположение |
|------|-------------|
| `Core/GameEngineDevice/.../StdLocalFileSystem.h/.cpp` | Core (общий, компилируется всегда) |
| `Platform/MacOS/.../StdLocalFileSystem.h/.cpp` | Platform (macOS-специфичная версия) |

> [!WARNING]
> **Линкер использует Core-версию.** Platform/MacOS версия компилируется, но
> её символы перезатираются Core версией (или не линкуются вовсе).
> Поэтому все изменения `StdLocalFileSystem` (search paths, `addSearchPath`,
> `resolveWithSearchPaths`) **должны вноситься в Core-версию**.

Аналогичная ситуация с `StdBIGFileSystem`:

| Файл | Расположение |
|------|-------------|
| `Core/GameEngineDevice/.../StdBIGFileSystem.cpp` | Core (используется линкером) |
| `Platform/MacOS/.../StdBIGFileSystem.cpp` | Platform (может не линковаться) |

## Патчи для macOS

### Case-Insensitive поиск файлов

`fixFilenameFromWindowsPath` обходит директории через `std::filesystem::directory_iterator`
и сравнивает имена через `strcasecmp`. Это нужно потому что:
- Игра запрашивает файлы в Windows-стиле: `Data\INI\GameData.ini`
- macOS может иметь case-sensitive файловую систему
- Реальный файл может иметь другой регистр

### Фильтрация дубликата INIZH.big

В `loadBigFilesFromDirectory` пропускается `INIZH.big` из подпапки `Data/INI/`,
потому что многие цифровые версии игры содержат дубликат этого файла,
что приводит к конфликту CRC в сетевой игре.

### Конвертация путей

Все обратные слеши `\` в путях автоматически заменяются на прямые `/` на этапе
открытия файла.

## Критические loose-файлы

Эти файлы **не запакованы** в `.big` и ищутся через search paths:

| Файл | Зачем |
|------|-------|
| `Data/Scripts/SkirmishScripts.scb` | AI скрипты для скирмиша (строительство, атаки) |
| `Data/Scripts/MultiplayerScripts.scb` | Скрипты для мультиплеера |
| `Data/INI/*.ini` | Конфигурация юнитов, оружия, зданий |

> [!CAUTION]
> Если `SkirmishScripts.scb` не найден, бот в скирмише **не строит и не атакует**.
> Это была основная проблема до внедрения `addSearchPath` в Core `StdLocalFileSystem`.
