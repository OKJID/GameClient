// MetalDevice8_state.h — private state members (included from MetalDevice8.h)
private:
  ULONG m_RefCount;

  void *m_Device;
  void *m_CommandQueue;
  void *m_MetalLayer;

  void *m_CurrentCommandBuffer;
  void *m_CurrentDrawable;
  void *m_CurrentEncoder;
  bool m_InScene;

  DWORD m_RenderStates[256];

  static const int MAX_TEXTURE_STAGES = 8;
  DWORD m_TextureStageStates[MAX_TEXTURE_STAGES][32];
  IDirect3DBaseTexture8 *m_Textures[MAX_TEXTURE_STAGES];
  uint32_t m_TextureGeneration[MAX_TEXTURE_STAGES];
  uint32_t m_TextureDirtyMask;

  D3DMATRIX m_Transforms[260];
  D3DVIEWPORT8 m_Viewport;
  D3DMATERIAL8 m_Material;
  float m_ClipPlanes[6][4];

  static const int MAX_LIGHTS = 8;
  D3DLIGHT8 m_Lights[MAX_LIGHTS];
  BOOL m_LightEnabled[MAX_LIGHTS];

  IDirect3DVertexBuffer8 *m_StreamSource;
  UINT m_StreamStride;
  IDirect3DIndexBuffer8 *m_IndexBuffer;
  UINT m_BaseVertexIndex;

  DWORD m_VertexShader;
  DWORD m_PixelShader;
  D3DGAMMARAMP m_GammaRamp;

  float m_VSConstants[MAX_VS_CONSTANTS][4];
  std::map<DWORD, VSHandleInfo> m_VSHandleMap;

  float m_PSConstants[MAX_PS_CONSTANTS][4];
  std::map<DWORD, PSHandleInfo> m_PSHandleMap;

  void *m_HWND;
  float m_ScreenWidth;
  float m_ScreenHeight;

  void *GetPSO(DWORD fvf, UINT stride);
  uint64_t BuildPSOKey(DWORD fvf, UINT stride);
  void *GetDepthStencilState();
  void CreateDepthTexture(UINT width, UINT height);
  void ApplyPerDrawState();
  void *GetSamplerState(DWORD stage);
  void BindUniforms(DWORD fvf);
  void BindCustomVSUniforms();
  void BindTexturesAndSamplers();
  static unsigned long MapPrimitiveType(DWORD d3dPrimType);

  void *m_Library;
  void *m_FunctionVertex;
  void *m_FunctionFragment;
  std::map<uint64_t, void *> m_PsoCache;

  void *m_DepthTexture;
  void *m_DepthStencilState;
  bool m_DepthStateDirty;
  bool m_DrawStateDirty;
  DWORD m_LastAppliedCull;
  DWORD m_LastAppliedZBias;
  std::map<uint32_t, void *> m_DepthStencilStateCache;

  std::map<uint32_t, void *> m_SamplerStateCache;

  void *m_ZeroBuffer;

  void *m_FrameSemaphore;
  static const int MAX_FRAMES_IN_FLIGHT = 2;

  MetalSurface8 *m_DefaultRTSurface;
  MetalSurface8 *m_DefaultDepthSurface;

  void *m_RTTColorTexture;
  void *m_RTTDepthTexture;
  IDirect3DSurface8 *m_RTTSurface;
  UINT m_RTTWidth;
  UINT m_RTTHeight;

  int m_MSAASampleCount;
  void *m_MSAAColorTexture;
  void *m_MSAADepthTexture;

  void *m_RingBuffer;
  uint32_t m_RingBufferSize;
  uint32_t m_RingBufferOffset;
