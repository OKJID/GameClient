#include "D3D8ShaderStages.h"
#include "D3D8ShaderOpcodes.h"

namespace D3D8Shader {

struct PendingWrite {
  Destination destination;
  std::string variable;
};

static bool validateRegisters(const ShaderEmitter &emitter, const Instruction &instruction, std::string &error) {
  if (instruction.opcode == OP_DCL || instruction.opcode == OP_DEF) {
    return true;
  }
  if (instruction.hasDestination) {
    bool operandIsRead = instruction.opcode == OP_TEXKILL && emitter.isPixel14();
    if (!emitter.checkRegister(instruction.destination.reg, !operandIsRead, error)) {
      return false;
    }
  }
  for (const Source &source : instruction.sources) {
    if (!emitter.checkRegister(source.reg, false, error)) {
      return false;
    }
  }
  return true;
}

static bool collectDefinitions(ShaderEmitter &emitter, std::string &error) {
  uint32_t limit = emitter.isPixel() ? MAX_PIXEL_CONSTANTS : MAX_VERTEX_CONSTANTS;
  for (const Instruction &instruction : emitter.program.instructions) {
    if (instruction.opcode != OP_DEF) {
      continue;
    }
    const Register &reg = instruction.destination.reg;
    if (reg.type != REG_CONST || reg.number >= limit) {
      error = "def must target a constant register below c" + std::to_string(limit);
      return false;
    }
    emitter.defineConstant(instruction);
  }
  return true;
}

static bool emitArithmetic(ShaderEmitter &emitter, const Instruction &instruction, size_t index,
                           std::vector<PendingWrite> &pending, std::string &error) {
  std::string expression;
  if (!arithmeticExpression(emitter, instruction, expression, error)) {
    return false;
  }

  std::string variable = "result" + std::to_string(index);
  emitter.line("float4 " + variable + " = " + emitter.finishResult(instruction.destination, expression) + ";");
  pending.push_back({instruction.destination, variable});
  return true;
}

static void flush(ShaderEmitter &emitter, std::vector<PendingWrite> &pending) {
  for (const PendingWrite &write : pending) {
    emitter.line(emitter.commit(write.destination, write.variable));
  }
  pending.clear();
}

static bool isDeclarative(Opcode opcode) {
  return opcode == OP_NOP || opcode == OP_DCL || opcode == OP_DEF || opcode == OP_PHASE;
}

static bool emitInstructions(ShaderEmitter &emitter, std::string &error) {
  const std::vector<Instruction> &instructions = emitter.program.instructions;
  TextureMatrixState matrix;
  std::vector<PendingWrite> pending;

  for (size_t i = 0; i < instructions.size(); ++i) {
    const Instruction &instruction = instructions[i];
    if (!validateRegisters(emitter, instruction, error)) {
      return false;
    }
    if (isDeclarative(instruction.opcode)) {
      continue;
    }

    if (isPixelTextureInstruction(instruction, emitter.program.version)) {
      flush(emitter, pending);
      if (!emitPixelTexture(emitter, instruction, matrix, error)) {
        return false;
      }
      continue;
    }

    if (!emitArithmetic(emitter, instruction, i, pending, error)) {
      return false;
    }
    bool pairedWithNext = i + 1 < instructions.size() && instructions[i + 1].coissue;
    if (!pairedWithNext) {
      flush(emitter, pending);
    }
  }
  flush(emitter, pending);
  return true;
}

static std::string declarationIfUsed(const ShaderEmitter &emitter, const char *name, const char *declaration) {
  return emitter.body().find(name) == std::string::npos ? "" : declaration;
}

static std::string vertexFunction(const ShaderEmitter &emitter, const std::string &name) {
  return "D3D8VertexShaderOutput " + name + "(thread const float4 *v, constant float4 *c) {\n"
    "    D3D8VertexShaderOutput o;\n"
    "    o.position = float4(0.0, 0.0, 0.0, 1.0);\n"
    "    o.diffuse = float4(1.0);\n"
    "    o.specular = float4(0.0);\n"
    "    for (int i = 0; i < 8; ++i) {\n"
    "        o.texCoord[i] = float4(0.0, 0.0, 0.0, 1.0);\n"
    "    }\n"
    "    o.fog = 1.0;\n"
    "    o.pointSize = 1.0;\n"
    + declarationIfUsed(emitter, "r[", "    float4 r[12] = {};\n")
    + declarationIfUsed(emitter, "a0", "    int4 a0 = int4(0);\n")
    + emitter.definedConstantsDeclaration() + emitter.body() +
    "    return o;\n"
    "}\n";
}

static std::string pixelFunction(const ShaderEmitter &emitter, const std::string &name) {
  return "D3D8PixelShaderOutput " + name + "(thread const D3D8PixelShaderInput &in, constant float4 *c,\n"
    "        constant D3D8PixelShaderState &state,\n"
    "        texture2d<float> tex0, texture2d<float> tex1, texture2d<float> tex2, texture2d<float> tex3,\n"
    "        texturecube<float> cube0, texturecube<float> cube1, texturecube<float> cube2, texturecube<float> cube3,\n"
    "        sampler s0, sampler s1, sampler s2, sampler s3) {\n"
    "    D3D8PixelShaderOutput o;\n"
    "    o.color = float4(0.0);\n"
    "    o.depth = 0.0;\n"
    "    o.killed = false;\n"
    "    float4 r[6] = {};\n"
    + declarationIfUsed(emitter, "t[", "    float4 t[4] = {};\n")
    + declarationIfUsed(emitter, "m3", "    float3 m3 = float3(0.0);\n")
    + emitter.definedConstantsDeclaration() + emitter.body() +
    "    o.color = r[0];\n"
    "    return o;\n"
    "}\n";
}

bool translate(const Program &program, const std::string &functionName, TranslatedShader &shader, std::string &error) {
  shader = TranslatedShader();
  shader.stage = program.version.stage;
  shader.functionName = functionName;

  ShaderEmitter emitter(program, shader);
  if (!collectDefinitions(emitter, error) || !emitInstructions(emitter, error)) {
    return false;
  }

  shader.source = emitter.isPixel() ? pixelFunction(emitter, functionName) : vertexFunction(emitter, functionName);
  return true;
}

} // namespace D3D8Shader
