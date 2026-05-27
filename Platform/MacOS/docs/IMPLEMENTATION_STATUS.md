# macOS Port — Implementation Status

> Updated: 2026-05-27

---

## Legend

| Symbol | Meaning |
|:---|:---|
| ✅ | Implemented (mirrors Windows flow) |
| ⚠️ | Partial implementation / safe stub |
| ❌ | Empty stub — potentially affects functionality |

---

## 1. DX8Wrapper (`dx8wrapper_metal.mm`)

### Critical Functions (affect rendering)

| Status | Function | Note |
|:---|:---|:---|
| ✅ | `Init()` | MetalInterface8 creation |
| ✅ | `Shutdown()` | Resource cleanup |
| ✅ | `Create_Device()` | MetalDevice8 via MetalInterface8::CreateDevice |
| ✅ | `Begin_Scene()` / `End_Scene()` | Full Metal frame lifecycle |
| ✅ | `Clear()` | Metal clear with correct flags |
| ✅ | `Draw()` | Apply_Render_State_Changes + DX8CALL(DrawIndexedPrimitive) |
| ✅ | `Draw_Triangles()` / `Draw_Strip()` | Delegate to Draw() |
| ✅ | `Draw_Sorting_IB_VB()` | Sorting renderer draw |
| ✅ | `Apply_Render_State_Changes()` | Full implementation per Windows |
| ✅ | `Set_World_Identity()` | Identity matrix in render_state |
| ✅ | `Set_View_Identity()` | Identity matrix in render_state |
| ✅ | `Set_Light()` | Light params → MetalDevice8 |
| ✅ | `Apply_Default_State()` | Default render states |
| ✅ | `Set_Gamma()` | CGSetDisplayTransferByTable |
| ✅ | `Invalidate_Cached_Render_States()` | Per Windows flow |
| ✅ | `Set_Render_Device()` | Resolution setup + Create_Device |
| ✅ | `Set_Device_Resolution()` | Dynamic resolution switching |
| ✅ | `Resize_And_Position_Window()` | NSWindow + CAMetalLayer + MetalDevice8 resize |
| ✅ | `Enumerate_Devices()` | Via MetalInterface8 |
| ✅ | `_Create_DX8_Texture()` | Via MetalDevice8::CreateTexture |
| ✅ | `Statistics (Reset/Begin/End)` | Frame statistics tracking |

### Non-Critical (do not affect rendering)

| Status | Function | Note |
|:---|:---|:---|
| ⚠️ | `Release_Device()` | Empty — resources freed in Shutdown |
| ⚠️ | `Reset_Device()` | Returns true — Metal does not lose devices |
| ⚠️ | `Toggle_Windowed()` | Empty — always windowed on macOS |
| ⚠️ | `Flip_To_Primary()` | Empty — no exclusive fullscreen |
| ⚠️ | `Set_Polygon_Mode()` | Empty — wireframe not used in game |
| ⚠️ | `Set_Swap_Interval()` | Empty — fps via FramePacer |
| ⚠️ | `Get_Format_Name()` | Empty — debug only |
| ⚠️ | `Get_DX8_Render_State_Value_Name()` | Empty — debug only |
| ⚠️ | `Get_DX8_Texture_Stage_State_Value_Name()` | Empty — debug only |

---

## 2. MetalDevice8 (`MetalDevice8.mm`)

| Status | Function | Note |
|:---|:---|:---|
| ✅ | `InitMetal()` | MTLDevice, CAMetalLayer, shaders, depth texture |
| ✅ | `BeginScene()` / `EndScene()` | Command buffer + drawable lifecycle |
| ✅ | `Clear()` | Render pass with clear/load actions |
| ✅ | `Present()` | presentDrawable + commit + waitUntilCompleted |
| ✅ | `DrawIndexedPrimitive()` | PSO + uniforms + textures + draw |
| ✅ | `DrawPrimitive()` | Non-indexed draw |
| ✅ | `DrawPrimitiveUP()` | Inline vertex data via setVertexBytes |
| ✅ | `SetTexture()` | Cache with generation tracking |
| ✅ | `SetRenderState()` | State cache for PSO rebuild |
| ✅ | `SetTextureStageState()` | TSS cache → fragment uniforms |
| ✅ | `SetTransform()` | Matrix storage in m_Transforms[260] |
| ✅ | `SetMaterial()` | Material storage → lighting uniforms |
| ✅ | `SetLight()` / `LightEnable()` | Light storage → lighting uniforms |
| ✅ | `SetViewport()` | Viewport + encoder update |
| ✅ | `SetStreamSource()` / `SetIndices()` | VB/IB binding |
| ✅ | `CreateTexture()` | MetalTexture8 with MTLBuffer backing |
| ✅ | `CreateVertexBuffer()` / `CreateIndexBuffer()` | MTLBuffer wrapper |
| ✅ | `SetRenderTarget()` | RTT mode with encoder restart |
| ✅ | `UpdateTexture()` | Blit encoder copy |
| ✅ | `SetGammaRamp()` | CGSetDisplayTransferByTable |
| ✅ | `GetPSO()` | Pipeline State Object cache |
| ✅ | `BindUniforms()` | 3 uniform buffers (vertex + fragment) |
| ✅ | `BindTexturesAndSamplers()` | 4 texture stages |
| ✅ | `GetSamplerState()` | Dynamic POINT→LINEAR promotion for CLAMP textures |
| ⚠️ | `CreatePixelShader()` | Dummy handle + bytecode classification (10 PS types) |
| ⚠️ | `CreateVertexShader()` | Dummy handle (FVF stored) |

---

## 3. MacOSGameEngine (`MacOSGameEngine.mm`)

| Status | Function | Note |
|:---|:---|:---|
| ✅ | `createGameLogic()` | W3DGameLogic |
| ✅ | `createGameClient()` | W3DGameClient |
| ✅ | `createModuleFactory()` | W3DModuleFactory |
| ✅ | `createThingFactory()` | W3DThingFactory |
| ✅ | `createFunctionLexicon()` | W3DFunctionLexicon |
| ✅ | `createLocalFileSystem()` | MacOSLocalFileSystem |
| ✅ | `createArchiveFileSystem()` | StdBIGFileSystem |
| ✅ | `createRadar()` | W3DRadar |
| ✅ | `createParticleSystemManager()` | W3DParticleSystemManager |
| ✅ | `createNetwork()` | NetworkInterface::createNetwork |
| ✅ | `createAudioManager()` | MacOSAudioManager (AVAudioEngine) |
| ⚠️ | `createWebBrowser()` | nullptr |

---

## 4. Input

| Status | Component | Note |
|:---|:---|:---|
| ✅ | `MacOSKeyboard` | NSEvent keyCode → game keys |
| ✅ | `MacOSMouse` | NSEvent mouse → game mouse events |
| ✅ | `serviceWindowsOS()` | NSEvent polling + CATransaction flush |

---

## 5. Audio

| Status | Component | Note |
|:---|:---|:---|
| ✅ | `MacOSAudioManager` | Full AudioManager: request queue, 64-source pool, priority eviction |
| ✅ | `AVAudioBridge` | C→ObjC bridge: AVAudioEngine, AVAudioPlayerNode, AVAudioEnvironmentNode |
| ✅ | `playAudioEvent` | 3D positional audio via `avbridge_play3D` (game sounds, weapons, vehicles) |
| ✅ | `friend_forcePlayAudioEventRTS` | 2D fire-and-forget via `avbridge_play` (UI sounds, lobby) |
| ✅ | `setDeviceListenerPosition` | Listener position/orientation → AVAudioEnvironmentNode |
| ✅ | WAV loading | PCM WAV from disk + `.big` archives, stereo→mono downmix for 3D |
| ✅ | Buffer cache | `m_bufferCache` prevents redundant disk reads |
| ✅ | Music streaming | MP3/AAC/ADPCM via `AVAudioFile` + `avbridge_playStream` |
| ✅ | Language fallback | `adjustForLocalization()` → English fallback for speech/music in non-English locales |
| ⚠️ | ADPCM WAV | Only PCM (format=1) supported; ADPCM files silently skipped |

---

## 6. Display

| Status | Component | Note |
|:---|:---|:---|
| ✅ | `MacOSDisplayManager` | `CGDisplayCopyAllDisplayModes` + standard mode generation |
| ✅ | `Resize_And_Position_Window` | NSWindow + CAMetalLayer + MetalDevice8 resize chain |
| ✅ | `windowDidEndLiveResize` | NSWindowDelegate bridge for resize events |

---

## 7. File System

| Status | Component | Note |
|:---|:---|:---|
| ✅ | `MacOSLocalFileSystem` | Slash normalization + case-insensitive lookup + search paths |
| ✅ | `DetectGameModes` | Auto-detects ZH and Base directories by marker files |
| ✅ | `registry.cpp` shim | Returns `basePath` for `InstallPath` key |
| ✅ | `fopen` interceptor | Normalizes `\` → `/` before calling real `fopen` |
| ✅ | `CreateDirectory` interceptor | Normalizes `\` → `/` before calling `mkdir` |
| ✅ | `.big` case-insensitive | `toLower()` on all archive keys in `m_files` map |

---

## 8. Multiplayer / Online

| Status | Component | Note |
|:---|:---|:---|
| ✅ | `GameNetworkingSockets` | Compiled for macOS ARM64, bundled as `.dylib` |
| ✅ | `libcurl` | HTTP/WS for lobby, auth, stats |
| ✅ | `OnlineServices_Auth` | `ShellExecuteA` → `system("open")`, token storage via file |
| ✅ | `OnlineServices_Init` | `GetModuleFileName` → shim, `LoadLibraryA` → `dlopen` |
| ✅ | `NGMPGame` / `NetworkMesh` | P2P mesh networking via GNS |
| ✅ | `Map lobby sync` | Custom map path resolution with `DEBUG_INFO_MAC` diagnostics |
| ✅ | `CRC compatibility` | Version manifest provides Windows-compatible exe CRC |
| ⚠️ | `Sentry` | Stub (crash reporting not ported) |

---

## 9. Deterministic Math

| Status | Component | Note |
|:---|:---|:---|
| ✅ | `fdlibm` integration | Sun's fdlibm 5.3 in `Core/Libraries/Source/WWVegas/WWMath/fdlibm/` |
| ✅ | `WWMath` refactor | All asm blocks replaced with `fdlibm_*()` calls |
| ✅ | Cross-platform parity | Bit-identical results on Windows x86/x64 and macOS ARM64 |

---

## 10. Compat Headers (`Include/`)

| File | Coverage |
|:---|:---|
| `windows.h` | HWND, HRESULT, MessageBox stubs, Win32 types, `fopen`/`CreateDirectory` interceptors |
| `d3d8_interfaces.h` | D3DMATRIX, D3DVIEWPORT8, D3DMATERIAL8, D3DLIGHT8 |
| `d3d8_structs.h` | D3DFMT, D3DRS, D3DTSS enums |
| `d3d8_com.h` | IDirect3DDevice8, IDirect3DTexture8 abstract interfaces |
| `d3dx8math.h` | D3DXMATRIX, D3DXVec3TransformCoord stubs |
| `dinput.h` | DirectInput types (DIDEVICEINSTANCE, etc.) |
| `ddraw.h` | DirectDraw types (DDSURFACEDESC, etc.) |
| `mss.h` | Miles Sound System types |
| `osdep.h` | Platform detection macros |

---

## 11. Video

| Status | Component | Note |
|:---|:---|:---|
| ✅ | `FFmpegVideoPlayer` | Bink → FFmpeg decoder for `.bik` playback (intro, campaign, challenge) |
| ✅ | `MacOSVideoAudioStream` | A/V sync via `AVAudioEngine` PCM scheduling |
| ✅ | Language fallback | `FFmpegVideoPlayer::open()` → English fallback for localized movie paths |

---

## 12. Launcher & Distribution

| Status | Component | Note |
|:---|:---|:---|
| ✅ | `GeneralsLauncher` (SwiftUI) | Folder picker + game data validation + launch with env vars |
| ✅ | Game Language picker | 11 languages, reads/writes `Options.ini`, scope `GLOBAL` |
| ✅ | Extended settings | Camera height/speed, FPS limiter, stats overlay, network, debug |
| ✅ | SteamCMD integration | Automatic asset download from Steam depots |
| ✅ | `assemble_distribution.sh` | 6-step pipeline: dylib bundling, rpath cleanup, launcher compile, asset injection, README, ZIP |
| ✅ | `dylibbundler` integration | Copies Homebrew `.dylib` deps → `Contents/Frameworks/`, rewrites `@rpath` |
| ✅ | `Info.plist` patching | Sets `CFBundleExecutable` to `GeneralsLauncher`, injects `AppIcon.png` |
| ✅ | Gatekeeper bypass README | Generated `README_INSTALL.md` with `xattr -cr` instructions |

