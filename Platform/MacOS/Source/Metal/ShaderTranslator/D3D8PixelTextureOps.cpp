#include "D3D8ShaderStages.h"
#include "D3D8ShaderOpcodes.h"

namespace D3D8Shader {

bool isPixelTextureInstruction(const Instruction &instruction, const Version &version) {
  if (version.stage != Stage::Pixel) {
    return false;
  }
  return isTextureAddressOpcode(instruction.opcode);
}

static bool sampleCall(ShaderEmitter &emitter, uint32_t stage, const std::string &coord, std::string &call,
                       std::string &error) {
  if (stage >= MAX_PIXEL_TEXTURE_STAGES) {
    error = "texture stage " + std::to_string(stage) + " is beyond the " + std::to_string(MAX_PIXEL_TEXTURE_STAGES)
      + " stages the device binds";
    return false;
  }

  emitter.shader.textureStageMask |= 1u << stage;
  std::string s = std::to_string(stage);
  call = "d3d8_sample(tex" + s + ", cube" + s + ", s" + s + ", state, " + s + "u, " + coord + ")";
  return true;
}

static std::string texCoord(uint32_t stage) {
  return "in.texCoord[" + std::to_string(stage) + "]";
}

static std::string textureRegister(uint32_t number) {
  return "t[" + std::to_string(number) + "]";
}

static std::string matrixRow(uint32_t stage, const Source &source) {
  return "dot(" + texCoord(stage) + ".xyz, " + textureRegister(source.reg.number) + ".xyz)";
}

static std::string bumpedCoord(uint32_t stage, const Source &source) {
  std::string matrix = "state.bumpEnvMatrix[" + std::to_string(stage) + "]";
  std::string offset = textureRegister(source.reg.number);
  return "float3(" + texCoord(stage) + ".x + " + matrix + ".x * " + offset + ".x + " + matrix + ".z * " + offset
    + ".y, " + texCoord(stage) + ".y + " + matrix + ".y * " + offset + ".x + " + matrix + ".w * " + offset
    + ".y, 0.0)";
}

static std::string projectedCoord(const Source &source, const std::string &value) {
  if (source.modifier == SRCMOD_DZ) {
    return "float3((" + value + ").xy / (" + value + ").z, (" + value + ").z)";
  }
  if (source.modifier == SRCMOD_DW) {
    return "((" + value + ").xyz / (" + value + ").w)";
  }
  return "(" + value + ").xyz";
}

static bool emitSample(ShaderEmitter &emitter, uint32_t stage, const std::string &coord, std::string &error) {
  std::string call;
  if (!sampleCall(emitter, stage, coord, call, error)) {
    return false;
  }
  emitter.line(textureRegister(stage) + " = " + call + ";");
  return true;
}

static bool emitPixel14Texture(ShaderEmitter &emitter, const Instruction &instruction, std::string &error) {
  const Destination &destination = instruction.destination;
  std::string target = "r[" + std::to_string(destination.reg.number) + "]";

  switch (instruction.opcode) {
    case OP_TEX: {
      const Source &source = instruction.sources[0];
      std::string call;
      if (!sampleCall(emitter, destination.reg.number, projectedCoord(source, emitter.registerValue(source.reg, false)),
                      call, error)) {
        return false;
      }
      emitter.line(emitter.commit(destination, call));
      return true;
    }
    case OP_TEXCOORD: {
      const Source &source = instruction.sources[0];
      emitter.line(emitter.commit(destination, "float4(" + projectedCoord(source, emitter.registerValue(source.reg, false))
                                                 + ", 1.0)"));
      return true;
    }
    case OP_TEXKILL:
      emitter.shader.usesTexkill = true;
      emitter.line("if (any(" + emitter.registerValue(destination.reg, false) + ".xyz < 0.0)) { o.killed = true; }");
      return true;
    case OP_TEXDEPTH:
      emitter.shader.writesDepth = true;
      emitter.line("o.depth = " + target + ".y == 0.0 ? 1.0 : " + target + ".x / " + target + ".y;");
      return true;
    default:
      error = std::string("unsupported ps.1.4 instruction ") + opcodeName(instruction.opcode, emitter.program.version);
      return false;
  }
}

static bool emitMatrixTexture(ShaderEmitter &emitter, const Instruction &instruction, TextureMatrixState &matrix,
                              std::string &error) {
  uint32_t stage = instruction.destination.reg.number;
  const Source &source = instruction.sources[0];
  std::string row = matrixRow(stage, source);
  static const char ROWS[] = "xyz";

  switch (instruction.opcode) {
    case OP_TEXM3x2PAD:
    case OP_TEXM3x3PAD:
      emitter.line(std::string("m3.") + ROWS[matrix.padRows % 3] + " = " + row + ";");
      ++matrix.padRows;
      return true;
    case OP_TEXM3x2TEX:
      matrix.padRows = 0;
      emitter.line("m3.y = " + row + ";");
      return emitSample(emitter, stage, "float3(m3.xy, 0.0)", error);
    case OP_TEXM3x2DEPTH:
      matrix.padRows = 0;
      emitter.shader.writesDepth = true;
      emitter.line("m3.y = " + row + ";");
      emitter.line("o.depth = m3.x == 0.0 ? 1.0 : m3.y / m3.x;");
      return true;
    case OP_TEXM3x3:
      matrix.padRows = 0;
      emitter.line("m3.z = " + row + ";");
      emitter.line(textureRegister(stage) + " = float4(m3, 1.0);");
      return true;
    case OP_TEXM3x3TEX:
      matrix.padRows = 0;
      emitter.line("m3.z = " + row + ";");
      return emitSample(emitter, stage, "m3", error);
    case OP_TEXM3x3SPEC:
      matrix.padRows = 0;
      emitter.line("m3.z = " + row + ";");
      return emitSample(emitter, stage, "d3d8_reflect(m3, (" + emitter.sourceValue(instruction.sources[1]) + ").xyz)",
                        error);
    default:
      matrix.padRows = 0;
      if (stage < 2) {
        error = "texm3x3vspec needs two pad rows before stage " + std::to_string(stage);
        return false;
      }
      emitter.line("m3.z = " + row + ";");
      return emitSample(emitter, stage, "d3d8_reflect(m3, float3(" + texCoord(stage - 2) + ".w, " + texCoord(stage - 1)
                                          + ".w, " + texCoord(stage) + ".w))", error);
  }
}

static bool emitRegisterTexture(ShaderEmitter &emitter, const Instruction &instruction, std::string &error) {
  uint32_t stage = instruction.destination.reg.number;
  const Source &source = instruction.sources[0];
  std::string offset = textureRegister(source.reg.number);

  switch (instruction.opcode) {
    case OP_TEXBEM:
      return emitSample(emitter, stage, bumpedCoord(stage, source), error);
    case OP_TEXBEML: {
      if (!emitSample(emitter, stage, bumpedCoord(stage, source), error)) {
        return false;
      }
      std::string luminance = "state.bumpEnvLuminance[" + std::to_string(stage) + "]";
      emitter.line(textureRegister(stage) + ".xyz *= saturate(" + offset + ".z * " + luminance + ".x + " + luminance
                   + ".y);");
      return true;
    }
    case OP_TEXREG2AR:
      return emitSample(emitter, stage, "float3(" + offset + ".w, " + offset + ".x, 0.0)", error);
    case OP_TEXREG2GB:
      return emitSample(emitter, stage, "float3(" + offset + ".y, " + offset + ".z, 0.0)", error);
    case OP_TEXREG2RGB:
      return emitSample(emitter, stage, offset + ".xyz", error);
    case OP_TEXDP3TEX:
      return emitSample(emitter, stage, "float3(" + matrixRow(stage, source) + ", 0.0, 0.0)", error);
    case OP_TEXDP3:
      emitter.line(textureRegister(stage) + " = float4(" + matrixRow(stage, source) + ");");
      return true;
    default:
      error = std::string("unsupported texture instruction ") + opcodeName(instruction.opcode, emitter.program.version);
      return false;
  }
}

bool emitPixelTexture(ShaderEmitter &emitter, const Instruction &instruction, TextureMatrixState &matrix,
                      std::string &error) {
  if (emitter.isPixel14()) {
    return emitPixel14Texture(emitter, instruction, error);
  }

  uint32_t stage = instruction.destination.reg.number;
  switch (instruction.opcode) {
    case OP_TEX:
      return emitSample(emitter, stage, "d3d8_projected(" + texCoord(stage) + ", state.projectionDivisor["
                                          + std::to_string(stage) + "])", error);
    case OP_TEXCOORD:
      emitter.line(textureRegister(stage) + " = float4(saturate(" + texCoord(stage) + ".xyz), 1.0);");
      return true;
    case OP_TEXKILL:
      emitter.shader.usesTexkill = true;
      emitter.line("if (any(" + texCoord(stage) + ".xyz < 0.0)) { o.killed = true; }");
      return true;
    case OP_TEXM3x2PAD:
    case OP_TEXM3x2TEX:
    case OP_TEXM3x2DEPTH:
    case OP_TEXM3x3PAD:
    case OP_TEXM3x3:
    case OP_TEXM3x3TEX:
    case OP_TEXM3x3SPEC:
    case OP_TEXM3x3VSPEC:
      return emitMatrixTexture(emitter, instruction, matrix, error);
    default:
      return emitRegisterTexture(emitter, instruction, error);
  }
}

} // namespace D3D8Shader
