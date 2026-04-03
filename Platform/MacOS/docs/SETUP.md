# macOS Port — Руководство по настройке

## Пререквизиты

| Требование | Версия | Примечание |
|:---|:---|:---|
| **macOS** | 13+ (Ventura) | Apple Silicon (ARM64) |
| **Xcode Command Line Tools** | Latest | `xcode-select --install` |
| **CMake** | 3.25+ | `brew install cmake` |
| **Ninja** | Latest | `brew install ninja` |
| **Игровые данные** | Generals: Zero Hour | `.big` файлы из установки |

## Сборка

### 1. Клонирование

```bash
git clone https://github.com/OKJID/GameClient.git
cd GameClient
git checkout okji/feat/macos-port
```

### 2. Сборка и запуск

```bash
sh build_run_mac.sh
```

Это автоматически:
- Конфигурирует CMake (preset `macos`, Ninja, Debug, ARM64)
- Собирает проект
- Запускает игру с перенаправлением логов

### 3. Игровые данные

Установите переменную `GENERALS_INSTALL_PATH` в `build_run_mac.sh`:

```bash
export GENERALS_INSTALL_PATH="/path/to/Command and Conquer - Generals/Command and Conquer Generals Zero Hour"
```

Директория должна содержать `.big` файлы:
`INIZH.big`, `W3DZH.big`, `TexturesZH.big`, `TerrainZH.big`, `WindowZH.big`, `ShadersZH.big`, `AudioZH.big`, etc.

## Логирование

Логи записываются в `Platform/MacOS/Build/Logs/game.log`.

Полезные паттерны:
- `[DIAG]` — диагностические сообщения рендеринга
- `[RFLOW:N]` — render flow (уровни 1-17)
- `[MetalDevice8]` — инициализация Metal
- `StdLocalFileSystem` — файловая система

## Команды

| Команда | Описание |
|:---|:---|
| `sh build_run_mac.sh` | Сборка + запуск |
| `sh build_run_mac.sh --clean` | Полная пересборка |
| `sh build_run_mac.sh --screenshot=N` | Скриншот через N секунд |
| `sh build_run_mac.sh --test` | Тесты Metal bridge |
| `sh build_run_mac.sh --lldb` | Запуск под дебаггером |

## Устранение проблем

### Игра не запускается
- Проверить `.big` файлы и `GENERALS_INSTALL_PATH`
- Убить зависшие процессы: `killall GeneralsOnlineZH`
- Пересобрать: `sh build_run_mac.sh --clean`

### Чёрный экран
- Проверить `game.log` на наличие `[MetalDevice8] Initialized:`
- Проверить `[DIAG] Present frame=N drawCalls=N` — draw calls > 0?
- Проверить `[DIAG] BindUniforms World:` — матрица НЕ нулевая?

### Magenta/фиолетовый экран
- Missing textures. Проверить `[MetalDevice8::CreateTexture]` в логе
- Проверить форматы: `fmt=21` (A8R8G8B8), `fmt=26` (A4R4G4B4)
