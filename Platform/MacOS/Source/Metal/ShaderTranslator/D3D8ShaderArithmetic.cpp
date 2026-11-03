#include "D3D8ShaderStages.h"
#include "D3D8ShaderOpcodes.h"

namespace D3D8Shader {

static std::string matrixProduct(const ShaderEmitter &emitter, const Instruction &instruction, uint32_t rows,
                                 bool fourComponents) {
  std::string vector = emitter.sourceValue(instruction.sources[0]);
  std::string result = "float4(";
  for (uint32_t row = 0; row < 4; ++row) {
    if (row > 0) {
      result += ", ";
    }
    if (row >= rows) {
      result += "0.0";
      continue;
    }
    std::string matrixRow = emitter.registerRow(instruction.sources[1], row);
    result += fourComponents ? "dot(" + vector + ", " + matrixRow + ")"
                             : "dot((" + vector + ").xyz, (" + matrixRow + ").xyz)";
  }
  return result + ")";
}

static std::string bumpEnvironmentExpression(const Instruction &instruction, const std::string &base,
                                             const std::string &offset) {
  std::string matrix = "state.bumpEnvMatrix[" + std::to_string(instruction.destination.reg.number) + "]";
  return "float4((" + base + ").x + " + matrix + ".x * (" + offset + ").x + " + matrix + ".z * (" + offset + ").y, ("
    + base + ").y + " + matrix + ".y * (" + offset + ").x + " + matrix + ".w * (" + offset + ").y, 0.0, 0.0)";
}

static std::string scalarExpression(Opcode opcode, const std::string &s0) {
  std::string scalar = "(" + s0 + ").w";
  switch (opcode) {
    case OP_RCP:
      return "float4(d3d8_rcp(" + scalar + "))";
    case OP_RSQ:
      return "float4(d3d8_rsq(" + scalar + "))";
    case OP_EXP:
      return "float4(exp2(" + scalar + "))";
    case OP_LOG:
      return "float4(d3d8_log(" + scalar + "))";
    case OP_EXPP:
      return "d3d8_expp(" + scalar + ")";
    default:
      return "d3d8_logp(" + scalar + ")";
  }
}

static bool vectorExpression(const ShaderEmitter &emitter, const Instruction &instruction,
                             const std::vector<std::string> &s, std::string &expression) {
  switch (instruction.opcode) {
    case OP_MOV:
      expression = s[0];
      return true;
    case OP_ADD:
      expression = "(" + s[0] + " + " + s[1] + ")";
      return true;
    case OP_SUB:
      expression = "(" + s[0] + " - " + s[1] + ")";
      return true;
    case OP_MUL:
      expression = "(" + s[0] + " * " + s[1] + ")";
      return true;
    case OP_MAD:
      expression = "(" + s[0] + " * " + s[1] + " + " + s[2] + ")";
      return true;
    case OP_DP3:
      expression = "float4(dot((" + s[0] + ").xyz, (" + s[1] + ").xyz))";
      return true;
    case OP_DP4:
      expression = "float4(dot(" + s[0] + ", " + s[1] + "))";
      return true;
    case OP_MIN:
      expression = "min(" + s[0] + ", " + s[1] + ")";
      return true;
    case OP_MAX:
      expression = "max(" + s[0] + ", " + s[1] + ")";
      return true;
    case OP_SLT:
      expression = "d3d8_slt(" + s[0] + ", " + s[1] + ")";
      return true;
    case OP_SGE:
      expression = "d3d8_sge(" + s[0] + ", " + s[1] + ")";
      return true;
    case OP_LRP:
      expression = "mix(" + s[2] + ", " + s[1] + ", " + s[0] + ")";
      return true;
    case OP_FRC:
      expression = "fract(" + s[0] + ")";
      return true;
    case OP_LIT:
      expression = "d3d8_lit(" + s[0] + ")";
      return true;
    case OP_DST:
      expression = "d3d8_dst(" + s[0] + ", " + s[1] + ")";
      return true;
    case OP_CMP:
      expression = "select(" + s[2] + ", " + s[1] + ", " + s[0] + " >= 0.0)";
      return true;
    case OP_CND:
      expression = emitter.isPixel14() ? "select(" + s[2] + ", " + s[1] + ", " + s[0] + " > 0.5)"
                                       : "((" + s[0] + ").w > 0.5 ? " + s[1] + " : " + s[2] + ")";
      return true;
    default:
      return false;
  }
}

static bool matrixExpression(const ShaderEmitter &emitter, const Instruction &instruction, std::string &expression) {
  switch (instruction.opcode) {
    case OP_M4x4:
      expression = matrixProduct(emitter, instruction, 4, true);
      return true;
    case OP_M4x3:
      expression = matrixProduct(emitter, instruction, 3, true);
      return true;
    case OP_M3x4:
      expression = matrixProduct(emitter, instruction, 4, false);
      return true;
    case OP_M3x3:
      expression = matrixProduct(emitter, instruction, 3, false);
      return true;
    case OP_M3x2:
      expression = matrixProduct(emitter, instruction, 2, false);
      return true;
    default:
      return false;
  }
}

bool arithmeticExpression(const ShaderEmitter &emitter, const Instruction &instruction, std::string &expression,
                          std::string &error) {
  std::vector<std::string> s;
  for (const Source &source : instruction.sources) {
    s.push_back(emitter.sourceValue(source));
  }

  if (isScalarOpcode(instruction.opcode)) {
    expression = scalarExpression(instruction.opcode, s[0]);
    return true;
  }
  if (instruction.opcode == OP_BEM) {
    expression = bumpEnvironmentExpression(instruction, s[0], s[1]);
    return true;
  }
  if (vectorExpression(emitter, instruction, s, expression) || matrixExpression(emitter, instruction, expression)) {
    return true;
  }

  error = std::string("unsupported instruction ") + opcodeName(instruction.opcode, emitter.program.version);
  return false;
}

} // namespace D3D8Shader
