#include "D3D8ShaderTokens.h"
#include "D3D8ShaderOpcodes.h"

#include <cstdio>
#include <cstring>

namespace D3D8Shader {

uint32_t Version::token() const {
  uint32_t base = stage == Stage::Vertex ? VERSION_VERTEX : VERSION_PIXEL;
  return base | (major << 8) | minor;
}

bool Version::isAtLeast(uint32_t wantedMajor, uint32_t wantedMinor) const {
  if (major != wantedMajor) {
    return major > wantedMajor;
  }
  return minor >= wantedMinor;
}

uint32_t swizzleComponent(uint32_t swizzle, uint32_t lane) {
  return (swizzle >> (2 * lane)) & 0x3;
}

static Register decodeRegister(uint32_t token) {
  Register reg;
  reg.type = (RegisterType)(((token >> 28) & 0x7) | ((token >> 8) & 0x18));
  reg.number = token & 0x7FF;
  return reg;
}

static uint32_t encodeRegister(const Register &reg) {
  return PARAMETER_BIT | ((reg.type & 0x7) << 28) | ((reg.type & 0x18) << 8) | (reg.number & 0x7FF);
}

static Destination decodeDestination(uint32_t token) {
  Destination destination;
  destination.reg = decodeRegister(token);
  destination.writeMask = (token >> 16) & 0xF;
  destination.saturate = (token & SATURATE_BIT) != 0;
  int32_t shift = (int32_t)((token >> 24) & 0xF);
  destination.shift = shift >= 8 ? shift - 16 : shift;
  return destination;
}

static uint32_t encodeDestination(const Destination &destination) {
  uint32_t token = encodeRegister(destination.reg) | (destination.writeMask << 16);
  if (destination.saturate) {
    token |= SATURATE_BIT;
  }
  return token | (((uint32_t)destination.shift & 0xF) << 24);
}

static Source decodeSource(uint32_t token) {
  Source source;
  source.reg = decodeRegister(token);
  source.swizzle = (token >> 16) & 0xFF;
  source.modifier = (SourceModifier)((token >> 24) & 0xF);
  source.relative = (token & RELATIVE_ADDRESS_BIT) != 0;
  return source;
}

static uint32_t encodeSource(const Source &source) {
  uint32_t token = encodeRegister(source.reg) | (source.swizzle << 16) | ((uint32_t)source.modifier << 24);
  if (source.relative) {
    token |= RELATIVE_ADDRESS_BIT;
  }
  return token;
}

static bool decodeVersion(uint32_t token, Version &version) {
  uint32_t kind = token & 0xFFFF0000;
  if (kind != VERSION_VERTEX && kind != VERSION_PIXEL) {
    return false;
  }
  version.stage = kind == VERSION_VERTEX ? Stage::Vertex : Stage::Pixel;
  version.major = (token >> 8) & 0xFF;
  version.minor = token & 0xFF;
  return version.major == 1;
}

static bool decodeInstruction(const uint32_t *tokens, size_t count, size_t &index, const Version &version,
                              Instruction &instruction, std::string &error) {
  uint32_t token = tokens[index++];
  instruction.opcode = (Opcode)(token & 0xFFFF);
  instruction.coissue = (token & COISSUE_BIT) != 0;

  OperandLayout layout;
  if (!operandLayout(instruction.opcode, version, layout)) {
    error = "unknown opcode " + std::to_string(instruction.opcode);
    return false;
  }

  if (index + layout.tokenCount() > count) {
    error = std::string("truncated ") + opcodeName(instruction.opcode, version);
    return false;
  }

  if (layout.declaration) {
    instruction.declaration = tokens[index++];
  }
  instruction.hasDestination = layout.destination;
  if (layout.destination) {
    instruction.destination = decodeDestination(tokens[index++]);
  }
  for (int i = 0; i < layout.sources; ++i) {
    instruction.sources.push_back(decodeSource(tokens[index++]));
  }
  if (layout.constant) {
    memcpy(instruction.constant, &tokens[index], sizeof(instruction.constant));
    index += 4;
  }
  return true;
}

bool decode(const uint32_t *tokens, size_t count, Program &program, std::string &error) {
  if (count == 0 || !decodeVersion(tokens[0], program.version)) {
    error = "not a 1.x shader version token";
    return false;
  }

  size_t index = 1;
  while (index < count) {
    uint32_t opcode = tokens[index] & 0xFFFF;
    if (tokens[index] == END_TOKEN) {
      return true;
    }
    if (opcode == OP_COMMENT) {
      index += 1 + ((tokens[index] >> 16) & 0x7FFF);
      continue;
    }

    Instruction instruction;
    if (!decodeInstruction(tokens, count, index, program.version, instruction, error)) {
      return false;
    }
    program.instructions.push_back(instruction);
  }

  error = "missing end token";
  return false;
}

std::vector<uint32_t> encode(const Program &program) {
  std::vector<uint32_t> tokens;
  tokens.push_back(program.version.token());

  for (const Instruction &instruction : program.instructions) {
    tokens.push_back(instruction.opcode | (instruction.coissue ? COISSUE_BIT : 0));
    if (instruction.opcode == OP_DCL) {
      tokens.push_back(instruction.declaration);
    }
    if (instruction.hasDestination) {
      tokens.push_back(encodeDestination(instruction.destination));
    }
    for (const Source &source : instruction.sources) {
      tokens.push_back(encodeSource(source));
    }
    if (instruction.opcode == OP_DEF) {
      uint32_t words[4];
      memcpy(words, instruction.constant, sizeof(words));
      tokens.insert(tokens.end(), words, words + 4);
    }
  }

  tokens.push_back(END_TOKEN);
  return tokens;
}

std::vector<uint32_t> withoutComments(const uint32_t *tokens, size_t count) {
  std::vector<uint32_t> result;
  Version version;
  if (count == 0 || !decodeVersion(tokens[0], version)) {
    return result;
  }

  result.push_back(tokens[0]);
  size_t index = 1;
  while (index < count) {
    uint32_t token = tokens[index];
    if (token == END_TOKEN) {
      result.push_back(token);
      return result;
    }
    if ((token & 0xFFFF) == OP_COMMENT) {
      index += 1 + ((token >> 16) & 0x7FFF);
      continue;
    }

    OperandLayout layout;
    if (!operandLayout((Opcode)(token & 0xFFFF), version, layout)) {
      return result;
    }
    size_t length = 1 + layout.tokenCount();
    for (size_t i = 0; i < length && index < count; ++i) {
      result.push_back(tokens[index++]);
    }
  }
  return result;
}

} // namespace D3D8Shader
