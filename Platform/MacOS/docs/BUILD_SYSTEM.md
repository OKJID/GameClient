# macOS Port — Система сборки

---

## Команды сборки

```bash
# Рекомендуемый способ (скрипт):
sh build_run_mac.sh              # configure + build + run
sh build_run_mac.sh --clean      # clean + configure + build + run
sh build_run_mac.sh --test       # build + run tests

# Ручная сборка (не рекомендуется):
cmake --preset macos
cmake --build build/macos
```

---

## Граф зависимостей

```
                    CMakeLists.txt (root)
                          │
  ┌───────────────────────┼───────────────────────────┐
  │     ЗАВИСИМОСТИ       │                           │
  │                       │                           │
  │  DX8 (APPLE) ─────────┼──► Platform/MacOS/Include │
  │    d3d8lib INTERFACE   │    d3d8_interfaces.h      │
  │    d3d8, d3dx8 empty   │    MetalDevice8 impl      │
  │                       │                           │
  │  GameSpy (APPLE) ──────┼──► INTERFACE only         │
  │  Miles (APPLE) ────────┼──► milesstub INTERFACE    │
  │  Bink (APPLE) ─────────┼──► binkstub INTERFACE     │
  │  Win32 libs (APPLE) ───┼──► INTERFACE dummies      │
  │  zlib (APPLE) ─────────┼──► System zlib            │
  │                       │                           │
  ├───────────────────────┼───────────────────────────┤
  │     ТАРГЕТЫ           │                           │
  │                       │                           │
  │  macos_platform ───────┼──► Platform/MacOS/        │
  │    STATIC library      │    MetalDevice8, MacOSMain│
  │    Links: Metal, AppKit│    Input, Audio, Stubs    │
  │      QuartzCore        │                           │
  │                       │                           │
  │  GeneralsOnlineZH ─────┼──► .app bundle            │
  │    Links: z_gameengine │                           │
  │      z_gameenginedevice│                           │
  │      macos_platform    │                           │
  │                       │                           │
  └───────────────────────┴───────────────────────────┘
```

---

## Ключевые CMake таргеты

| Таргет | Тип | Выход | Описание |
|:---|:---|:---|:---|
| `macos_platform` | STATIC | `libmacos_platform.a` | Весь macOS-специфичный код |
| `GeneralsOnlineZH` | EXECUTABLE | `.app` bundle | Zero Hour |
| `z_gameengine` | STATIC | `libz_gameengine.a` | ZH engine library |
| `z_gameenginedevice` | STATIC | `libz_gameenginedevice.a` | ZH device library |
| `metal_bridge_tests` | EXECUTABLE | `Tests/metal_bridge_tests` | Unit-тесты Metal bridge |

---

## macOS Framework Dependencies

| Framework | Назначение |
|:---|:---|
| `Metal` | GPU рендеринг |
| `MetalKit` | Metal утилиты |
| `AppKit` | Управление окнами, события |
| `QuartzCore` | `CAMetalLayer` |
| `CoreGraphics` | Gamma ramp |

---

## Preset конфигурация

`CMakePresets.json` определяет пресет `macos`:

```json
{
  "name": "macos",
  "generator": "Ninja",
  "binaryDir": "build/macos",
  "cacheVariables": {
    "CMAKE_BUILD_TYPE": "Debug",
    "CMAKE_OSX_ARCHITECTURES": "arm64"
  }
}
```

---

## Переменные окружения

| Переменная | Описание | Default |
|:---|:---|:---|
| `GENERALS_INSTALL_PATH` | Путь к установке игры | (обязательная) |
| `GENERALS_FPS_LIMIT` | Лимит FPS | 60 |
| `GENERALS_MSAA` | MSAA sample count | 1 (off) |
