# macOS Port — Руководство разработчика

---

## Золотые правила

1. **Shared код** (`Core/`, `GeneralsMD/Code/`) — модифицировать только под `#ifdef __APPLE__`. Не `#ifdef _WIN32`
2. **Платформенный код** — свободно в `Platform/MacOS/`
3. **Повторять Windows flow** — не костыли, а оттестированное поведение Windows
4. **GeneralsGameCode** (`/Users/okji/dev/games/GeneralsGameCode/`) — **только справочник**, не копировать напрямую
5. **Сборка и запуск** — всегда через `sh build_run_mac.sh`
6. **Логирование** — `printf` + `fflush(stdout)`. НЕ `fprintf(stderr)` (stdout перенаправляется в game.log)
7. **DLOG_RFLOW(level, fmt, ...)** — для категоризированных логов Metal backend
8. **Все костыли** помечать `TODO(PS_PATH):` с описанием
9. **Scope** — только `GeneralsMD/` (Zero Hour). `Generals/` НЕ поддерживается

---

## Архитектура

### Стратегия: Гибрид A+B

`DX8Wrapper` остаётся с тем же именем и API:
- `#ifndef __APPLE__` → оригинальная DX8-реализация (Windows)  
- `#ifdef __APPLE__` → Metal-реализация в `dx8wrapper_metal.mm`

Весь WW3D2 код (152 файла) — **без изменений**.

### Компоненты

| Подсистема | Файлы | Назначение |
|:---|:---|:---|
| **Metal Backend** | `Source/Metal/MetalDevice8.mm` (~2900 строк) + 5 пар .h/.mm | DX8 → Metal мост |
| **DX8Wrapper Metal** | `Source/Metal/dx8wrapper_metal.mm` (~1700 строк) | Статический класс с кэшированием |
| **Entry Point** | `Source/Main/MacOSMain.mm` | NSApplication, GameMain, CreateGameEngine |
| **Game Engine** | `Source/Main/MacOSGameEngine.mm` | Фабрика подсистем (W3DGameLogic, StdFileSystem, etc.) |
| **Input** | `Source/Input/MacOSKeyboard.cpp`, `MacOSMouse.cpp` | NSEvent → game input |
| **Audio** | `Source/Audio/MacOSAudioManager.h` | Stub AudioManager |
| **Shaders** | `Source/Metal/MacOSShaders.metal` | FFP эмуляция |
| **Compat Headers** | `Include/windows.h`, `d3d8*.h`, etc. | Заглушки Win32/D3D типов |

### GameEngine фабрика

```cpp
class MacOSGameEngine : public GameEngine {
    GameLogic*          createGameLogic()         → W3DGameLogic
    GameClient*         createGameClient()        → W3DGameClient
    ModuleFactory*      createModuleFactory()     → W3DModuleFactory
    LocalFileSystem*    createLocalFileSystem()   → StdLocalFileSystem   // ← не Win32
    ArchiveFileSystem*  createArchiveFileSystem()  → StdBIGFileSystem     // ← не Win32
    AudioManager*       createAudioManager()      → MacOSAudioManager    // ← stub
    WebBrowser*         createWebBrowser()         → nullptr
    // Остальное — идентично Win32GameEngine
};
```

---

## Подводные камни (Gotchas)

### 1. DX8Wrapper: Deferred State Application
`Set_Transform(WORLD/VIEW)` НЕ вызывает `D3DDevice->SetTransform` сразу. Матрицы сохраняются в `render_state.world/view` и применяются в `Apply_Render_State_Changes()` перед каждым `Draw()`.

**Критично:** Если функция типа `Set_World_Identity()` пустая заглушка → `render_state.world` остаётся нулевой → чёрный экран.

### 2. D3D→Metal матрицы
D3D row-major `memcpy` в Metal column-major `float4x4` = транспонирование. Шейдер: `P * V * W * pos` = эквивалент D3D `pos * W * V * P`.

### 3. NSApplication + dispatch_async
`[NSApp run]` запускает event loop. `dispatch_async(main_queue)` ставит game loop в очередь. Game loop блокирует main queue (бесконечный цикл). `serviceWindowsOS()` вручную качает события через `[NSApp nextEventMatchingMask:]`. `[CATransaction flush]` нужен для обновления окна.

### 4. Файловая система сканирует CWD
Игра запускается из корня исходного кода. `StdLocalFileSystem` сканирует `.` = тысячи файлов. Нужно исправить рабочую директорию.

### 5. FVF Stride vs Offset
`GetPSO` использует stride от вызывающего кода, а НЕ вычисляет из FVF. C++ структуры могут иметь padding.

---

## Сборка и запуск

```bash
sh build_run_mac.sh                  # сборка + запуск
sh build_run_mac.sh --clean          # полная пересборка
sh build_run_mac.sh --screenshot=N   # скриншот через N секунд
sh build_run_mac.sh --test           # тесты Metal bridge
sh build_run_mac.sh --lldb           # запуск под дебаггером
```

### Файлы логов

| Файл | Что пишет | Как активировать |
|:---|:---|:---|
| `Platform/MacOS/Build/Logs/game.log` | stdout игры (`printf`, `DEBUG_INFO_MAC`) | Всегда (stdout → pipe) |
| `~/Command and Conquer Generals Zero Hour Data/\GeneralsOnlineData\GeneralsOnline.log` | `NetworkLog()` — сетевой трафик, HTTP, ICE, mesh | Всегда (⚠ путь с backslash — см. ниже) |
| `CRCLogs/DebugFrame_*.txt` | Покадровый CRC dump | `--saveDebugCRCPerFrame ./CRCLogs` |
| `Platform/MacOS/Build/Logs/screenshot_game_window.png` | Скриншот | `--screenshot=N` |

> **⚠ GeneralsOnline.log — backslash path quirk:**
> `NetworkLog` формирует путь через `std::format("{}\\GeneralsOnlineData\\GeneralsOnline.log", UserData)`.
> На macOS `\` не является разделителем папок — файл создаётся с **литеральным backslash в имени**.
> Путь:  
> `~/Command and Conquer Generals Zero Hour Data/\GeneralsOnlineData\GeneralsOnline.log`
>
> Для чтения в терминале:
> ```bash
> cat ~/Command\ and\ Conquer\ Generals\ Zero\ Hour\ Data/\\GeneralsOnlineData\\GeneralsOnline.log
> ```

---

## DEBUG_INFO_MAC — Система диагностики

### Активация

```bash
export GENERALS_MAC_DEBUG=1
```

Макрос `DEBUG_INFO_MAC((fmt, ...))` определён в `Core/GameEngine/Include/Common/Debug.h`.
На macOS: `printf("[DEBUG_INFO_MAC] " fmt "\n"); fflush(stdout)` → попадает в `game.log`.
На не-Apple: `((void)0)` — no-op. **Не нужен `#ifdef __APPLE__`.**

### Теги по подсистемам

#### Сетевое лобби и map transfer

| Тег | Файл | Что логирует |
|:---|:---|:---|
| `[ROOM_DATA]` | `OnlineServices_LobbyInterface.cpp` | Map path correction, member parsing (uid/hasMap/slot), ignoring updates during gameplay, SyncWithLobby calls |
| `[SYNC_LOBBY]` | `NGMPGame.cpp` | Map resolution: OFFICIAL / CUSTOM / FALLBACK с путями |
| `[SLOT_SYNC]` | `NGMPGame.cpp` | Каждый слот при синхронизации (uid/hasMap/name), `m_inProgress` блок, AI слоты |
| `[GAME_START]` | `WOLGameSetupMenu.cpp` | Все слоты `myGame` vs `TheNGMPGame` ДО и ПОСЛЕ `*TheNGMPGame = *myGame` (адреса объектов) |
| `[START_GAME]` | `NGMPGame.cpp` | Вход в `startGame()`, `m_inProgress`, `m_inGame`, все human-слоты |
| `[LAUNCH]` | `NGMPGame.cpp` | Все слоты перед `DoAnyMapTransfers`, результат, `findMap` путь/результат, BAIL |
| `[MAP_XFER]` | `FileTransfer.cpp` | Каждый слот в mask-loop (isHuman/hasMap), итоговый mask |

#### CRC / Out-of-Sync

| Тег | Файл | Что логирует |
|:---|:---|:---|
| `[CRC_CHECK]` | `GameLogic.cpp` | Validator CRC, каждый player CRC, MISMATCH/ok, missing CRCs |

#### Рендеринг

| Тег | Файл | Что логирует |
|:---|:---|:---|
| `[DIAG]` | Metal backend | Present, BeginScene, DrawCalls, матрицы, viewport, TSS, readback |

### Пример вывода в game.log

```
[DEBUG_INFO_MAC] [ROOM_DATA] parsed member slot[1]: uid=52117 name='dima ok' hasMap=0 state=5
[DEBUG_INFO_MAC] [SYNC_LOBBY] Map resolved as CUSTOM: 'data/maps/! 1v1 cxn/! 1v1 cxn.map'
[DEBUG_INFO_MAC] [SLOT_SYNC] slot[1] uid=52117 hasMap=0 name='dima ok'
[DEBUG_INFO_MAC] [GAME_START] === Before copy: myGame slots ===
[DEBUG_INFO_MAC] [GAME_START] myGame slot[1]: state=5 isHuman=1 hasMap=0
[DEBUG_INFO_MAC] [GAME_START] === After copy: TheNGMPGame slots ===
[DEBUG_INFO_MAC] [GAME_START] TheNGMPGame slot[1]: state=5 isHuman=1 hasMap=???  <-- key moment
[DEBUG_INFO_MAC] [MAP_XFER] slot[1]: isHuman=1 hasMap=??? 
[DEBUG_INFO_MAC] [MAP_XFER] mask=0x??? map='...'
[DEBUG_INFO_MAC] [CRC_CHECK] frame=100 validating CRCs
[DEBUG_INFO_MAC] [CRC_CHECK] validator player[0] CRC=0x1A2B3C4D (numCRCs=2 numPlayers=2)
[DEBUG_INFO_MAC] [CRC_CHECK] player[1] CRC=0x1A2B3C4D vs validator=0x1A2B3C4D ok
```

---

## CRC Debug (покадровый)

Подключается через CLI флаг в `build_run_mac.sh`:
```
-saveDebugCRCPerFrame /path/to/CRCLogs
```

Генерирует `DebugFrame_NNNN.txt` для каждого фрейма с полным state dump.
`NET_CRC_INTERVAL` = 100 (release) / 1 (debug) — интервал сверки CRC между клиентами.

---

## NetworkLog — Сетевые логи

`NetworkLog(ELogVerbosity, fmt, ...)` определён в `NGMP_Helpers.cpp`.
Пишет в `GeneralsOnline.log` (см. quirk выше).

### Уровни

| Уровень | Когда пишет |
|:---|:---|
| `LOG_RELEASE` | Всегда (ошибки, drop пакетов, disconnect) |
| `LOG_DEBUG` | Только если `Debug_VerboseLogging()` = true |

### Что покрывает

- HTTP запросы/ответы к `api.playgenerals.online`
- ICE/P2P mesh — подключение, disconnect, signaling
- Game packet recv/send — размеры, drop reasons, buffer overflow
- Lobby sync polling
- `[PRESEED]` — latency seeding

**Не удалять логи** — нужны для дальнейшей отладки.
