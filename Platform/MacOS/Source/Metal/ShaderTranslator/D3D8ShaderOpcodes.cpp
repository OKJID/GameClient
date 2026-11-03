#include "D3D8ShaderOpcodes.h"

#include <cstring>

namespace D3D8Shader {

struct OpcodeInfo {
  Opcode opcode;
  const char *name;
  bool destination;
  int sources;
};

static const OpcodeInfo OPCODES[] = {
  {OP_NOP, "nop", false, 0},
  {OP_MOV, "mov", true, 1},
  {OP_ADD, "add", true, 2},
  {OP_SUB, "sub", true, 2},
  {OP_MAD, "mad", true, 3},
  {OP_MUL, "mul", true, 2},
  {OP_RCP, "rcp", true, 1},
  {OP_RSQ, "rsq", true, 1},
  {OP_DP3, "dp3", true, 2},
  {OP_DP4, "dp4", true, 2},
  {OP_MIN, "min", true, 2},
  {OP_MAX, "max", true, 2},
  {OP_SLT, "slt", true, 2},
  {OP_SGE, "sge", true, 2},
  {OP_EXP, "exp", true, 1},
  {OP_LOG, "log", true, 1},
  {OP_LIT, "lit", true, 1},
  {OP_DST, "dst", true, 2},
  {OP_LRP, "lrp", true, 3},
  {OP_FRC, "frc", true, 1},
  {OP_M4x4, "m4x4", true, 2},
  {OP_M4x3, "m4x3", true, 2},
  {OP_M3x4, "m3x4", true, 2},
  {OP_M3x3, "m3x3", true, 2},
  {OP_M3x2, "m3x2", true, 2},
  {OP_DCL, "dcl", true, 0},
  {OP_TEXCOORD, "texcoord", true, 0},
  {OP_TEXKILL, "texkill", true, 0},
  {OP_TEX, "tex", true, 0},
  {OP_TEXBEM, "texbem", true, 1},
  {OP_TEXBEML, "texbeml", true, 1},
  {OP_TEXREG2AR, "texreg2ar", true, 1},
  {OP_TEXREG2GB, "texreg2gb", true, 1},
  {OP_TEXM3x2PAD, "texm3x2pad", true, 1},
  {OP_TEXM3x2TEX, "texm3x2tex", true, 1},
  {OP_TEXM3x3PAD, "texm3x3pad", true, 1},
  {OP_TEXM3x3TEX, "texm3x3tex", true, 1},
  {OP_TEXM3x3SPEC, "texm3x3spec", true, 2},
  {OP_TEXM3x3VSPEC, "texm3x3vspec", true, 1},
  {OP_EXPP, "expp", true, 1},
  {OP_LOGP, "logp", true, 1},
  {OP_CND, "cnd", true, 3},
  {OP_DEF, "def", true, 0},
  {OP_TEXREG2RGB, "texreg2rgb", true, 1},
  {OP_TEXDP3TEX, "texdp3tex", true, 1},
  {OP_TEXM3x2DEPTH, "texm3x2depth", true, 1},
  {OP_TEXDP3, "texdp3", true, 1},
  {OP_TEXM3x3, "texm3x3", true, 1},
  {OP_TEXDEPTH, "texdepth", true, 0},
  {OP_CMP, "cmp", true, 3},
  {OP_BEM, "bem", true, 2},
  {OP_PHASE, "phase", false, 0},
};

static const OpcodeInfo *findOpcode(Opcode opcode) {
  for (const OpcodeInfo &info : OPCODES) {
    if (info.opcode == opcode) {
      return &info;
    }
  }
  return nullptr;
}

static bool isPixelShader14(const Version &version) {
  return version.stage == Stage::Pixel && version.isAtLeast(1, 4);
}

bool operandLayout(Opcode opcode, const Version &version, OperandLayout &layout) {
  const OpcodeInfo *info = findOpcode(opcode);
  if (!info) {
    return false;
  }

  layout.declaration = opcode == OP_DCL;
  layout.destination = info->destination;
  layout.sources = info->sources;
  layout.constant = opcode == OP_DEF;

  if ((opcode == OP_TEX || opcode == OP_TEXCOORD) && isPixelShader14(version)) {
    layout.sources = 1;
  }
  return true;
}

const char *opcodeName(Opcode opcode, const Version &version) {
  if (isPixelShader14(version)) {
    if (opcode == OP_TEX) {
      return "texld";
    }
    if (opcode == OP_TEXCOORD) {
      return "texcrd";
    }
  }

  const OpcodeInfo *info = findOpcode(opcode);
  return info ? info->name : "?";
}

bool opcodeFromName(const std::string &name, Opcode &opcode) {
  if (name == "texld") {
    opcode = OP_TEX;
    return true;
  }
  if (name == "texcrd") {
    opcode = OP_TEXCOORD;
    return true;
  }

  for (const OpcodeInfo &info : OPCODES) {
    if (name == info.name) {
      opcode = info.opcode;
      return true;
    }
  }
  return false;
}

size_t OperandLayout::tokenCount() const {
  return (declaration ? 1 : 0) + (destination ? 1 : 0) + sources + (constant ? 4 : 0);
}

static const char *const USAGE_NAMES[] = {
  "position", "blendweight", "blendindices", "normal", "psize", "texcoord", "tangent",
  "binormal", "tessfactor", "positiont", "color", "fog", "depth", "sample",
};

std::string declarationUsageName(uint32_t declaration) {
  uint32_t usage = declaration & 0x1F;
  uint32_t index = (declaration >> 16) & 0xF;
  std::string name = usage < sizeof(USAGE_NAMES) / sizeof(USAGE_NAMES[0]) ? USAGE_NAMES[usage] : "usage" + std::to_string(usage);
  return index == 0 ? name : name + std::to_string(index);
}

bool declarationFromUsageName(const std::string &name, uint32_t &declaration) {
  size_t digits = name.find_first_of("0123456789");
  std::string usageName = name.substr(0, digits);
  uint32_t index = digits == std::string::npos ? 0 : (uint32_t)std::stoul(name.substr(digits));

  for (uint32_t usage = 0; usage < sizeof(USAGE_NAMES) / sizeof(USAGE_NAMES[0]); ++usage) {
    if (usageName == USAGE_NAMES[usage]) {
      declaration = PARAMETER_BIT | usage | (index << 16);
      return true;
    }
  }
  return false;
}

bool isTextureAddressOpcode(Opcode opcode) {
  return (opcode >= OP_TEXCOORD && opcode <= OP_TEXM3x3VSPEC)
    || (opcode >= OP_TEXREG2RGB && opcode <= OP_TEXDEPTH);
}

bool isScalarOpcode(Opcode opcode) {
  switch (opcode) {
    case OP_RCP:
    case OP_RSQ:
    case OP_EXP:
    case OP_LOG:
    case OP_EXPP:
    case OP_LOGP:
      return true;
    default:
      return false;
  }
}

} // namespace D3D8Shader
