#include <d3dx8math.h>

#include "ShaderTranslator/D3D8ShaderAssembler.h"
#include "../Utils/MacDebug.h"

HRESULT D3DXAssembleShader(const char *pSrcData, UINT SrcDataLen, DWORD Flags, void *pConstants, ID3DXBuffer **ppShader,
                           void **ppErrorMsgs) {
  if (ppErrorMsgs) {
    *ppErrorMsgs = nullptr;
  }
  if (!pSrcData || !ppShader) {
    return E_POINTER;
  }

  D3D8Shader::Program program;
  std::string error;
  if (!D3D8Shader::assemble(pSrcData, SrcDataLen, program, error)) {
    DEBUG_RENDERING_MAC(("D3DXAssembleShader: %s", error.c_str()));
    *ppShader = nullptr;
    return E_FAIL;
  }

  std::vector<uint32_t> tokens = D3D8Shader::encode(program);
  *ppShader = new DummyD3DXBuffer((const DWORD *)tokens.data(), (DWORD)(tokens.size() * sizeof(uint32_t)));
  return S_OK;
}
