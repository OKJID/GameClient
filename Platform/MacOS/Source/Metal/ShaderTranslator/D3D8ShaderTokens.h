#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace D3D8Shader {

enum class Stage { Vertex, Pixel };

enum Opcode : uint32_t {
  OP_NOP = 0,
  OP_MOV = 1,
  OP_ADD = 2,
  OP_SUB = 3,
  OP_MAD = 4,
  OP_MUL = 5,
  OP_RCP = 6,
  OP_RSQ = 7,
  OP_DP3 = 8,
  OP_DP4 = 9,
  OP_MIN = 10,
  OP_MAX = 11,
  OP_SLT = 12,
  OP_SGE = 13,
  OP_EXP = 14,
  OP_LOG = 15,
  OP_LIT = 16,
  OP_DST = 17,
  OP_LRP = 18,
  OP_FRC = 19,
  OP_M4x4 = 20,
  OP_M4x3 = 21,
  OP_M3x4 = 22,
  OP_M3x3 = 23,
  OP_M3x2 = 24,
  OP_DCL = 31,
  OP_TEXCOORD = 64,
  OP_TEXKILL = 65,
  OP_TEX = 66,
  OP_TEXBEM = 67,
  OP_TEXBEML = 68,
  OP_TEXREG2AR = 69,
  OP_TEXREG2GB = 70,
  OP_TEXM3x2PAD = 71,
  OP_TEXM3x2TEX = 72,
  OP_TEXM3x3PAD = 73,
  OP_TEXM3x3TEX = 74,
  OP_TEXM3x3SPEC = 76,
  OP_TEXM3x3VSPEC = 77,
  OP_EXPP = 78,
  OP_LOGP = 79,
  OP_CND = 80,
  OP_DEF = 81,
  OP_TEXREG2RGB = 82,
  OP_TEXDP3TEX = 83,
  OP_TEXM3x2DEPTH = 84,
  OP_TEXDP3 = 85,
  OP_TEXM3x3 = 86,
  OP_TEXDEPTH = 87,
  OP_CMP = 88,
  OP_BEM = 89,
  OP_PHASE = 0xFFFD,
  OP_COMMENT = 0xFFFE,
  OP_END = 0xFFFF,
};

enum RegisterType : uint32_t {
  REG_TEMP = 0,
  REG_INPUT = 1,
  REG_CONST = 2,
  REG_ADDR_OR_TEXTURE = 3,
  REG_RASTOUT = 4,
  REG_ATTROUT = 5,
  REG_TEXCRDOUT = 6,
};

enum RasterOutput : uint32_t {
  RASTOUT_POSITION = 0,
  RASTOUT_FOG = 1,
  RASTOUT_POINT_SIZE = 2,
};

enum SourceModifier : uint32_t {
  SRCMOD_NONE = 0,
  SRCMOD_NEG = 1,
  SRCMOD_BIAS = 2,
  SRCMOD_BIASNEG = 3,
  SRCMOD_SIGN = 4,
  SRCMOD_SIGNNEG = 5,
  SRCMOD_COMP = 6,
  SRCMOD_X2 = 7,
  SRCMOD_X2NEG = 8,
  SRCMOD_DZ = 9,
  SRCMOD_DW = 10,
};

constexpr uint32_t VERSION_VERTEX = 0xFFFE0000;
constexpr uint32_t VERSION_PIXEL = 0xFFFF0000;
constexpr uint32_t END_TOKEN = 0x0000FFFF;
constexpr uint32_t PARAMETER_BIT = 0x80000000;
constexpr uint32_t COISSUE_BIT = 0x40000000;
constexpr uint32_t RELATIVE_ADDRESS_BIT = 0x00002000;
constexpr uint32_t SATURATE_BIT = 0x00100000;
constexpr uint32_t IDENTITY_SWIZZLE = 0xE4;
constexpr uint32_t FULL_WRITE_MASK = 0xF;

struct Version {
  Stage stage = Stage::Pixel;
  uint32_t major = 0;
  uint32_t minor = 0;

  uint32_t token() const;
  bool isAtLeast(uint32_t wantedMajor, uint32_t wantedMinor) const;
};

struct Register {
  RegisterType type = REG_TEMP;
  uint32_t number = 0;
};

struct Destination {
  Register reg;
  uint32_t writeMask = FULL_WRITE_MASK;
  bool saturate = false;
  int32_t shift = 0;
};

struct Source {
  Register reg;
  uint32_t swizzle = IDENTITY_SWIZZLE;
  SourceModifier modifier = SRCMOD_NONE;
  bool relative = false;
};

struct Instruction {
  Opcode opcode = OP_NOP;
  bool coissue = false;
  bool hasDestination = false;
  Destination destination;
  std::vector<Source> sources;
  float constant[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  uint32_t declaration = 0;
};

struct Program {
  Version version;
  std::vector<Instruction> instructions;
};

uint32_t swizzleComponent(uint32_t swizzle, uint32_t lane);

bool decode(const uint32_t *tokens, size_t count, Program &program, std::string &error);
std::string disassemble(const Program &program);
std::vector<uint32_t> encode(const Program &program);
std::vector<uint32_t> withoutComments(const uint32_t *tokens, size_t count);

} // namespace D3D8Shader
