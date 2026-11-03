#pragma once

#include "D3D8ShaderTokens.h"

namespace D3D8Shader {

struct OperandLayout {
  bool declaration = false;
  bool destination = false;
  int sources = 0;
  bool constant = false;

  size_t tokenCount() const;
};

bool operandLayout(Opcode opcode, const Version &version, OperandLayout &layout);
const char *opcodeName(Opcode opcode, const Version &version);
bool opcodeFromName(const std::string &name, Opcode &opcode);

std::string declarationUsageName(uint32_t declaration);
bool declarationFromUsageName(const std::string &name, uint32_t &declaration);

bool isTextureAddressOpcode(Opcode opcode);
bool isScalarOpcode(Opcode opcode);

} // namespace D3D8Shader
