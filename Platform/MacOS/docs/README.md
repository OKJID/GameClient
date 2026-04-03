# macOS Port — Документация

> **Command & Conquer: Generals — Zero Hour** на Apple Silicon (ARM64)
> 
> Ветка: `okji/feat/macos-port` | Репозиторий: GameClient (чистая реализация)

## Документы

| Документ | Описание |
|:---|:---|
| **[Руководство по настройке](SETUP.md)** | Пререквизиты, сборка, запуск |
| **[Руководство разработчика](DEVELOPMENT.md)** | Архитектура, правила, подводные камни |
| **[Конвейер рендеринга](RENDERING.md)** | Metal backend, трансляция DX8->Metal, шейдеры |
| **[Система сборки](BUILD_SYSTEM.md)** | CMake структура, зависимости, таргеты |
| **[Аудит заглушек](STUBS_AUDIT.md)** | Статус реализации каждого компонента |

## Быстрый старт

```bash
# Сборка + запуск (рекомендуется)
sh build_run_mac.sh

# С захватом скриншота через 15 секунд
sh build_run_mac.sh --screenshot=15

# Чистая пересборка
sh build_run_mac.sh --clean

# Запуск тестов Metal bridge
sh build_run_mac.sh --test
```

## Текущий статус

| Метрика | Значение |
|:---|:---|
| **Сборка** | Собирается — `GeneralsOnlineZH.app` |
| **Рантайм** | Стабилен — 496+ кадров без крашей |
| **Game Loop** | Работает — GameMain -> GameEngine::update |
| **Рендеринг** | Частично — геометрия и UI видны, текстуры не загружены (magenta), изображение зеркально |
| **Аудио** | Заглушка (MacOSAudioManager) |
| **Ввод** | MacOSKeyboard + MacOSMouse через NSEvent |
| **Shell Map** | Загружается, но фон не виден (missing textures) |

## Архитектура

### Стратегия: Гибрид A+B (DX8Wrapper с Metal-реализацией)

Оставляем `dx8wrapper.h` с тем же именем класса `DX8Wrapper`, но:
- `#ifndef __APPLE__` — оригинальная DX8-реализация (Windows)
- `#ifdef __APPLE__` — Metal-реализация с **идентичным public API**

Весь потребительский код (WW3D2, W3DDevice, ShaderManager) вызывает
`DX8Wrapper::Begin_Scene()`, `DX8Wrapper::Draw_Triangles()` и т.д. —
**ни один include, ни один вызов менять не нужно**.

### Структура файлов

```
Platform/MacOS/
├── CMakeLists.txt                     # macOS cmake target
├── Build/
│   ├── Logs/game.log                  # Лог игры (stdout перенаправлен)
│   └── screenshot.py                  # Утилита скриншотов
├── Include/                           # Compat-заголовки (d3d8, windows.h, etc.)
├── Source/
│   ├── Main/
│   │   ├── MacOSMain.mm              # Entry point (NSApplication + GameMain)
│   │   ├── MacOSGameEngine.h/.mm     # GameEngine фабрика подсистем
│   │   └── MacOSDebugLog.h           # DLOG_RFLOW макрос
│   ├── Metal/
│   │   ├── dx8wrapper_metal.mm       # DX8Wrapper Metal-реализация (~1700 строк)
│   │   ├── MetalDevice8.h/.mm        # IDirect3DDevice8 -> Metal (~2900 строк)
│   │   ├── MetalInterface8.h/.mm     # IDirect3D8 -> Metal
│   │   ├── MetalTexture8.h/.mm       # IDirect3DTexture8 -> MTLTexture
│   │   ├── MetalSurface8.h/.mm       # IDirect3DSurface8 -> staging buffer
│   │   ├── MetalVertexBuffer8.h/.mm  # VB -> MTLBuffer
│   │   ├── MetalIndexBuffer8.h/.mm   # IB -> MTLBuffer
│   │   └── MacOSShaders.metal        # FFP эмуляция (vertex + fragment)
│   ├── Input/
│   │   ├── MacOSKeyboard.h/.cpp      # NSEvent -> Keyboard
│   │   └── MacOSMouse.h/.cpp         # NSEvent -> Mouse
│   ├── Audio/
│   │   └── MacOSAudioManager.h       # Stub AudioManager
│   └── GeneralsOnlineStubs.cpp       # Стабы сетевых сервисов
└── docs/                              # <- Вы здесь
```

## Scope

- **Только `GeneralsMD/`** (Zero Hour). `Generals/` (ванильные) НЕ поддерживается.
- Тулзы (WorldBuilder и т.д.) НЕ собираются на macOS.
- Сеть/мультиплеер — заглушки, не функциональны.

## Ключевые правила

1. **Shared код** (`Core/`, `GeneralsMD/Code/`) — модифицировать только под `#ifdef __APPLE__`
2. **Платформенный код** — свободно в `Platform/MacOS/`
3. **Сборка и запуск** — всегда через `sh build_run_mac.sh`
4. **Повторять Windows flow** — не костыли, а оттестированное флоу
5. **GeneralsGameCode** — использовать только как справочник
