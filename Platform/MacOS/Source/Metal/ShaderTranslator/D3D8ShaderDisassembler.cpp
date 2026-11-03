#include "D3D8ShaderTokens.h"
#include "D3D8ShaderOpcodes.h"

#include <cstdio>

namespace D3D8Shader {

static const char COMPONENTS[] = "xyzw";

static std::string registerName(const Register &reg, const Version &version) {
  std::string number = std::to_string(reg.number);
  bool pixel = version.stage == Stage::Pixel;

  switch (reg.type) {
    case REG_TEMP:
      return "r" + number;
    case REG_INPUT:
      return "v" + number;
    case REG_CONST:
      return "c" + number;
    case REG_ADDR_OR_TEXTURE:
      return (pixel ? "t" : "a") + number;
    case REG_RASTOUT:
      if (reg.number == RASTOUT_FOG) {
        return "oFog";
      }
      return reg.number == RASTOUT_POINT_SIZE ? "oPts" : "oPos";
    case REG_ATTROUT:
      return "oD" + number;
    case REG_TEXCRDOUT:
      return "oT" + number;
  }
  return "?" + number;
}

static std::string writeMaskText(uint32_t writeMask) {
  if (writeMask == FULL_WRITE_MASK) {
    return "";
  }

  std::string text = ".";
  for (uint32_t lane = 0; lane < 4; ++lane) {
    if (writeMask & (1u << lane)) {
      text += COMPONENTS[lane];
    }
  }
  return text;
}

static std::string swizzleText(uint32_t swizzle) {
  if (swizzle == IDENTITY_SWIZZLE) {
    return "";
  }

  std::string text = ".";
  for (uint32_t lane = 0; lane < 4; ++lane) {
    text += COMPONENTS[swizzleComponent(swizzle, lane)];
  }
  return text;
}

static const char *modifierSuffix(SourceModifier modifier) {
  switch (modifier) {
    case SRCMOD_BIAS:
    case SRCMOD_BIASNEG:
      return "_bias";
    case SRCMOD_SIGN:
    case SRCMOD_SIGNNEG:
      return "_bx2";
    case SRCMOD_X2:
    case SRCMOD_X2NEG:
      return "_x2";
    case SRCMOD_DZ:
      return "_dz";
    case SRCMOD_DW:
      return "_dw";
    default:
      return "";
  }
}

static const char *modifierPrefix(SourceModifier modifier) {
  switch (modifier) {
    case SRCMOD_NEG:
    case SRCMOD_BIASNEG:
    case SRCMOD_SIGNNEG:
    case SRCMOD_X2NEG:
      return "-";
    case SRCMOD_COMP:
      return "1-";
    default:
      return "";
  }
}

static std::string sourceText(const Source &source, const Version &version) {
  std::string reg = registerName(source.reg, version);
  if (source.relative) {
    reg = "c[a0.x + " + std::to_string(source.reg.number) + "]";
  }
  return modifierPrefix(source.modifier) + reg + modifierSuffix(source.modifier) + swizzleText(source.swizzle);
}

static std::string shiftText(int32_t shift) {
  switch (shift) {
    case 1:
      return "_x2";
    case 2:
      return "_x4";
    case 3:
      return "_x8";
    case -1:
      return "_d2";
    case -2:
      return "_d4";
    case -3:
      return "_d8";
    default:
      return "";
  }
}

static std::string floatText(float value) {
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%.9g", value);
  return buffer;
}

static std::string instructionText(const Instruction &instruction, const Version &version) {
  std::string text = instruction.coissue ? "+" : "";
  text += opcodeName(instruction.opcode, version);
  if (instruction.opcode == OP_DCL) {
    text += "_" + declarationUsageName(instruction.declaration);
  }

  if (instruction.hasDestination) {
    text += shiftText(instruction.destination.shift);
    if (instruction.destination.saturate) {
      text += "_sat";
    }
    text += " " + registerName(instruction.destination.reg, version) + writeMaskText(instruction.destination.writeMask);
  }

  for (const Source &source : instruction.sources) {
    text += ", " + sourceText(source, version);
  }

  if (instruction.opcode == OP_DEF) {
    for (float value : instruction.constant) {
      text += ", " + floatText(value);
    }
  }
  return text;
}

std::string disassemble(const Program &program) {
  const Version &version = program.version;
  std::string text = version.stage == Stage::Vertex ? "vs." : "ps.";
  text += std::to_string(version.major) + "." + std::to_string(version.minor) + "\n";

  for (const Instruction &instruction : program.instructions) {
    text += instructionText(instruction, version) + "\n";
  }
  return text;
}

} // namespace D3D8Shader
