#pragma once

#include "D3D8ShaderTokens.h"

namespace D3D8Shader {

constexpr uint32_t MAX_VERTEX_INPUTS = 16;
constexpr uint32_t MAX_VERTEX_CONSTANTS = 96;
constexpr uint32_t MAX_VERTEX_TEMPS = 12;
constexpr uint32_t MAX_TEXCOORD_OUTPUTS = 8;
constexpr uint32_t MAX_PIXEL_CONSTANTS = 8;
constexpr uint32_t MAX_PIXEL_TEMPS = 6;
constexpr uint32_t MAX_PIXEL_TEXTURE_STAGES = 4;

struct TranslatedShader {
  Stage stage = Stage::Pixel;
  std::string functionName;
  std::string source;
  uint32_t inputRegisterMask = 0;
  uint32_t textureStageMask = 0;
  bool writesFog = false;
  bool writesPointSize = false;
  bool writesDepth = false;
  bool usesTexkill = false;
};

const char *shaderPrelude();
bool translate(const Program &program, const std::string &functionName, TranslatedShader &shader, std::string &error);

} // namespace D3D8Shader
