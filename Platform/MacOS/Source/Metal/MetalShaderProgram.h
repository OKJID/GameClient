#pragma once

#include <windows.h>
#include <d3d8.h>

#include "ShaderTranslator/D3D8ShaderTranslator.h"

#include <string>
#include <vector>

struct VertexElement {
  uint32_t reg;
  uint32_t type;
  uint32_t offset;
};

// A DX8 shader translated to Metal. A vertex shader pairs with either a translated pixel shader
// or fragment_main; a pixel shader pairs with either a translated vertex shader or vertex_main.
class MetalShaderProgram {
public:
  D3D8Shader::TranslatedShader shader;
  std::vector<VertexElement> elements;
  void *library = nullptr;
  void *functionWithShaderPartner = nullptr;
  void *functionWithFixedFunctionPartner = nullptr;

  ~MetalShaderProgram();

  static MetalShaderProgram *createVertexShader(void *device, const DWORD *declaration, const DWORD *function,
                                                std::string &error);
  static MetalShaderProgram *createPixelShader(void *device, const DWORD *function, std::string &error);
};
