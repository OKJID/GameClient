# macOS Stubs Audit — Текущий статус

**Обновлено:** 2026-04-03

---

## Легенда

| Символ | Значение |
|:---|:---|
| ✅ | Реализовано по Windows flow |
| ⚠️ | Частичная реализация / безопасная заглушка |
| ❌ | Пустая заглушка — потенциально влияет на работу |

---

## 1. DX8Wrapper (`dx8wrapper_metal.mm`)

### Критические функции (влияют на рендер)

| Статус | Функция | Примечание |
|:---|:---|:---|
| ✅ | `Init()` | MetalInterface8 создание |
| ✅ | `Shutdown()` | Очистка ресурсов |
| ✅ | `Create_Device()` | MetalDevice8 через MetalInterface8::CreateDevice |
| ✅ | `Begin_Scene()` / `End_Scene()` | Полный Metal frame lifecycle |
| ✅ | `Clear()` | Metal clear с правильными флагами |
| ✅ | `Draw()` | Apply_Render_State_Changes + DX8CALL(DrawIndexedPrimitive) |
| ✅ | `Draw_Triangles()` / `Draw_Strip()` | Делегируют в Draw() |
| ✅ | `Draw_Sorting_IB_VB()` | Sorting renderer draw |
| ✅ | `Apply_Render_State_Changes()` | Полная реализация по Windows |
| ✅ | `Set_World_Identity()` | **БЫЛО ❌ → ИСПРАВЛЕНО 2026-04-03** |
| ✅ | `Set_View_Identity()` | **БЫЛО ❌ → ИСПРАВЛЕНО 2026-04-03** |
| ✅ | `Set_Light()` | **БЫЛО ❌ → ИСПРАВЛЕНО 2026-04-03** |
| ✅ | `Apply_Default_State()` | **БЫЛО ❌ → ИСПРАВЛЕНО 2026-04-03** |
| ✅ | `Set_Gamma()` | **БЫЛО ❌ → ИСПРАВЛЕНО 2026-04-03** |
| ✅ | `Invalidate_Cached_Render_States()` | По Windows flow |
| ✅ | `Set_Render_Device()` | Настройка разрешения + Create_Device |
| ✅ | `Enumerate_Devices()` | Через MetalInterface8 |
| ✅ | `_Create_DX8_Texture()` | Через MetalDevice8::CreateTexture |
| ✅ | `Statistics (Reset/Begin/End)` | **БЫЛО ❌ → ИСПРАВЛЕНО 2026-04-03** |

### Некритические (не влияют на рендер)

| Статус | Функция | Примечание |
|:---|:---|:---|
| ⚠️ | `Release_Device()` | Пустая — ресурсы освобождаются в Shutdown |
| ⚠️ | `Reset_Device()` | Возвращает true — Metal не теряет устройство |
| ⚠️ | `Toggle_Windowed()` | Пустая — всегда windowed на macOS |
| ⚠️ | `Resize_And_Position_Window()` | Пустая |
| ⚠️ | `Flip_To_Primary()` | Пустая — нет exclusive fullscreen |
| ⚠️ | `Set_Polygon_Mode()` | Пустая — wireframe не используется в игре |
| ⚠️ | `Set_Swap_Interval()` | Пустая — fps через FramePacer |
| ⚠️ | `Get_Format_Name()` | Пустая — только для отладки |
| ⚠️ | `Get_DX8_Render_State_Value_Name()` | Пустая — отладка |
| ⚠️ | `Get_DX8_Texture_Stage_State_Value_Name()` | Пустая — отладка |

---

## 2. MetalDevice8 (`MetalDevice8.mm`)

| Статус | Функция | Примечание |
|:---|:---|:---|
| ✅ | `InitMetal()` | MTLDevice, CAMetalLayer, shaders, depth texture |
| ✅ | `BeginScene()` / `EndScene()` | Command buffer + drawable lifecycle |
| ✅ | `Clear()` | Render pass с clear/load actions |
| ✅ | `Present()` | presentDrawable + commit + waitUntilCompleted |
| ✅ | `DrawIndexedPrimitive()` | PSO + uniforms + textures + draw |
| ✅ | `DrawPrimitive()` | Аналогично без index buffer |
| ✅ | `DrawPrimitiveUP()` | Inline vertex data через setVertexBytes |
| ✅ | `SetTexture()` | Кэш с generation tracking |
| ✅ | `SetRenderState()` | State cache для PSO rebuild |
| ✅ | `SetTextureStageState()` | TSS cache → fragment uniforms |
| ✅ | `SetTransform()` | Matrix storage в m_Transforms[260] |
| ✅ | `SetMaterial()` | Material storage → lighting uniforms |
| ✅ | `SetLight()` / `LightEnable()` | Light storage → lighting uniforms |
| ✅ | `SetViewport()` | Viewport + encoder update |
| ✅ | `SetStreamSource()` / `SetIndices()` | VB/IB binding |
| ✅ | `CreateTexture()` | MetalTexture8 с MTLBuffer backing |
| ✅ | `CreateVertexBuffer()` / `CreateIndexBuffer()` | MTLBuffer wrapper |
| ✅ | `SetRenderTarget()` | RTT mode с encoder restart |
| ✅ | `UpdateTexture()` | Blit encoder copy |
| ✅ | `SetGammaRamp()` | CGSetDisplayTransferByTable |
| ✅ | `GetPSO()` | Pipeline State Object кэш |
| ✅ | `BindUniforms()` | 3 uniform buffers (vertex + fragment) |
| ✅ | `BindTexturesAndSamplers()` | 4 texture stages |
| ⚠️ | `CreatePixelShader()` | Dummy handle + bytecode classification |
| ⚠️ | `CreateVertexShader()` | Dummy handle (FVF stored) |

---

## 3. MacOSGameEngine (`MacOSGameEngine.mm`)

| Статус | Функция | Примечание |
|:---|:---|:---|
| ✅ | `createGameLogic()` | W3DGameLogic |
| ✅ | `createGameClient()` | W3DGameClient |
| ✅ | `createModuleFactory()` | W3DModuleFactory |
| ✅ | `createThingFactory()` | W3DThingFactory |
| ✅ | `createFunctionLexicon()` | W3DFunctionLexicon |
| ✅ | `createLocalFileSystem()` | StdLocalFileSystem |
| ✅ | `createArchiveFileSystem()` | StdBIGFileSystem |
| ✅ | `createRadar()` | W3DRadar |
| ✅ | `createParticleSystemManager()` | W3DParticleSystemManager |
| ✅ | `createNetwork()` | NetworkInterface::createNetwork |
| ⚠️ | `createAudioManager()` | MacOSAudioManager (stub) |
| ⚠️ | `createWebBrowser()` | nullptr |

---

## 4. Ввод (Input)

| Статус | Компонент | Примечание |
|:---|:---|:---|
| ✅ | `MacOSKeyboard` | NSEvent keyCode → game keys |
| ✅ | `MacOSMouse` | NSEvent mouse → game mouse events |
| ✅ | `serviceWindowsOS()` | NSEvent polling + CATransaction flush |

---

## 5. Аудио

| Статус | Компонент | Примечание |
|:---|:---|:---|
| ⚠️ | `MacOSAudioManager` | Stub — все методы no-op, предотвращает null-ptr crashes |

---

## 6. Compat Headers (`Include/`)

| Файл | Что покрывает |
|:---|:---|
| `windows.h` | HWND, HRESULT, MessageBox stubs, Win32 types |
| `d3d8_interfaces.h` | D3DMATRIX, D3DVIEWPORT8, D3DMATERIAL8, D3DLIGHT8 |
| `d3d8_structs.h` | D3DFMT, D3DRS, D3DTSS enums |
| `d3d8_com.h` | IDirect3DDevice8, IDirect3DTexture8 abstract interfaces |
| `d3dx8math.h` | D3DXMATRIX, D3DXVec3TransformCoord stubs |
| `dinput.h` | DirectInput types (DIDEVICEINSTANCE, etc.) |
| `ddraw.h` | DirectDraw types (DDSURFACEDESC, etc.) |
| `mss.h` | Miles Sound System types |
| `osdep.h` | Platform detection macros |
