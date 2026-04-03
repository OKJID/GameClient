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
- `Platform/MacOS/Build/Logs/game.log` — stdout игры
- `Platform/MacOS/Build/Logs/screenshot_game_window.png` — скриншот (при `--screenshot`)

---

## Диагностические логи (текущие)

В коде расставлены `[DIAG]` логи для отладки рендеринга:
- `[DIAG] Present frame=N drawable=... drawCalls=N` — каждый фрейм
- `[DIAG] BeginScene: got drawable=... texture=... WxH` — получение drawable
- `[DIAG] DrawIndexedPrimitive SKIPPED` — пропущенные draw calls (если VB/IB=null)
- `[DIAG] BindUniforms fvf=... useProj=...` — матрицы (каждые 120 фреймов)
- `[DIAG] SetTransform WORLD/VIEW/PROJ` — все изменения матриц
- `[DIAG] SetViewport` — все изменения viewport
- `[DIAG] TSS` — texture stage states (каждые 120 фреймов)
- `[DIAG] CENTER_PIXEL / TL / BR` — readback пикселей (каждые 60 фреймов)

**Не удалять** — нужны для дальнейшей отладки.
