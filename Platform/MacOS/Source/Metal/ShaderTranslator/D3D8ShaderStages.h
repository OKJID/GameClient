#pragma once

#include "D3D8ShaderEmitter.h"

namespace D3D8Shader {

struct TextureMatrixState {
  uint32_t padRows = 0;
};

bool isPixelTextureInstruction(const Instruction &instruction, const Version &version);
bool arithmeticExpression(const ShaderEmitter &emitter, const Instruction &instruction, std::string &expression,
                          std::string &error);
bool emitPixelTexture(ShaderEmitter &emitter, const Instruction &instruction, TextureMatrixState &matrix,
                      std::string &error);

} // namespace D3D8Shader
