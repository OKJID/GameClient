#include "D3D8ShaderEmitter.h"

#include <cmath>
#include <cstdio>

namespace D3D8Shader {

static const char COMPONENTS[] = "xyzw";

std::string swizzleSuffix(uint32_t swizzle) {
  if (swizzle == IDENTITY_SWIZZLE) {
    return "";
  }
  std::string suffix = ".";
  for (uint32_t lane = 0; lane < 4; ++lane) {
    suffix += COMPONENTS[swizzleComponent(swizzle, lane)];
  }
  return suffix;
}

std::string writeMaskSuffix(uint32_t writeMask) {
  std::string suffix = ".";
  for (uint32_t lane = 0; lane < 4; ++lane) {
    if (writeMask & (1u << lane)) {
      suffix += COMPONENTS[lane];
    }
  }
  return suffix;
}

std::string floatLiteral(float value) {
  if (std::isnan(value)) {
    return "NAN";
  }
  if (std::isinf(value)) {
    return value > 0 ? "INFINITY" : "(-INFINITY)";
  }
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%.9g", value);
  std::string text = buffer;
  if (text.find_first_of(".e") == std::string::npos) {
    text += ".0";
  }
  return text;
}

ShaderEmitter::ShaderEmitter(const Program &program, TranslatedShader &shader) : program(program), shader(shader) {}

bool ShaderEmitter::isPixel() const {
  return program.version.stage == Stage::Pixel;
}

bool ShaderEmitter::isPixel14() const {
  return isPixel() && program.version.isAtLeast(1, 4);
}

static bool vertexRegisterInRange(const Register &reg, bool destination) {
  switch (reg.type) {
    case REG_TEMP:
      return reg.number < MAX_VERTEX_TEMPS;
    case REG_INPUT:
      return !destination && reg.number < MAX_VERTEX_INPUTS;
    case REG_CONST:
      return !destination && reg.number < MAX_VERTEX_CONSTANTS;
    case REG_ADDR_OR_TEXTURE:
      return reg.number == 0;
    case REG_RASTOUT:
      return destination && reg.number <= RASTOUT_POINT_SIZE;
    case REG_ATTROUT:
      return destination && reg.number < 2;
    case REG_TEXCRDOUT:
      return destination && reg.number < MAX_TEXCOORD_OUTPUTS;
  }
  return false;
}

static bool pixelRegisterInRange(const Register &reg, bool destination, bool pixel14) {
  switch (reg.type) {
    case REG_TEMP:
      return reg.number < MAX_PIXEL_TEMPS;
    case REG_INPUT:
      return !destination && reg.number < 2;
    case REG_CONST:
      return !destination && reg.number < MAX_PIXEL_CONSTANTS;
    case REG_ADDR_OR_TEXTURE:
      if (pixel14) {
        return !destination && reg.number < MAX_TEXCOORD_OUTPUTS;
      }
      return reg.number < MAX_PIXEL_TEXTURE_STAGES;
    default:
      return false;
  }
}

bool ShaderEmitter::checkRegister(const Register &reg, bool destination, std::string &error) const {
  bool valid = isPixel() ? pixelRegisterInRange(reg, destination, isPixel14()) : vertexRegisterInRange(reg, destination);
  if (!valid) {
    error = std::string(destination ? "destination" : "source") + " register type " + std::to_string(reg.type)
      + " number " + std::to_string(reg.number) + " is out of range";
  }
  return valid;
}

std::string ShaderEmitter::constantValue(uint32_t number, bool relative) const {
  if (!relative) {
    auto defined = definedConstants.find(number);
    if (defined != definedConstants.end()) {
      return "k" + std::to_string(number);
    }
    return "c[" + std::to_string(number) + "]";
  }

  std::string index = "a0.x + " + std::to_string(number);
  std::string value = "d3d8_vsConstant(c, " + index + ")";
  for (const auto &defined : definedConstants) {
    value = "(" + index + " == " + std::to_string(defined.first) + " ? k" + std::to_string(defined.first) + " : " + value + ")";
  }
  return value;
}

std::string ShaderEmitter::vertexRegisterValue(const Register &reg, bool relative) const {
  std::string number = std::to_string(reg.number);
  switch (reg.type) {
    case REG_TEMP:
      return "r[" + number + "]";
    case REG_INPUT:
      shader.inputRegisterMask |= 1u << reg.number;
      return "v[" + number + "]";
    case REG_CONST:
      return constantValue(reg.number, relative);
    case REG_ADDR_OR_TEXTURE:
      return "float4(a0)";
    default:
      return "float4(0.0)";
  }
}

std::string ShaderEmitter::pixelRegisterValue(const Register &reg) const {
  std::string number = std::to_string(reg.number);
  switch (reg.type) {
    case REG_TEMP:
      return "r[" + number + "]";
    case REG_INPUT:
      return reg.number == 0 ? "in.diffuse" : "in.specular";
    case REG_CONST:
      return constantValue(reg.number, false);
    case REG_ADDR_OR_TEXTURE:
      return isPixel14() ? "in.texCoord[" + number + "]" : "t[" + number + "]";
    default:
      return "float4(0.0)";
  }
}

std::string ShaderEmitter::registerValue(const Register &reg, bool relative) const {
  return isPixel() ? pixelRegisterValue(reg) : vertexRegisterValue(reg, relative);
}

std::string ShaderEmitter::registerRow(const Source &source, uint32_t row) const {
  Register reg = source.reg;
  reg.number += row;
  return registerValue(reg, source.relative);
}

std::string ShaderEmitter::sourceValue(const Source &source) const {
  std::string value = registerValue(source.reg, source.relative) + swizzleSuffix(source.swizzle);
  switch (source.modifier) {
    case SRCMOD_NEG:
      return "(-" + value + ")";
    case SRCMOD_BIAS:
      return "(" + value + " - 0.5)";
    case SRCMOD_BIASNEG:
      return "(0.5 - " + value + ")";
    case SRCMOD_SIGN:
      return "(2.0 * " + value + " - 1.0)";
    case SRCMOD_SIGNNEG:
      return "(1.0 - 2.0 * " + value + ")";
    case SRCMOD_COMP:
      return "(1.0 - " + value + ")";
    case SRCMOD_X2:
      return "(2.0 * " + value + ")";
    case SRCMOD_X2NEG:
      return "(-2.0 * " + value + ")";
    default:
      return value;
  }
}

std::string ShaderEmitter::destinationTarget(const Register &reg) const {
  std::string number = std::to_string(reg.number);
  switch (reg.type) {
    case REG_TEMP:
      return "r[" + number + "]";
    case REG_ADDR_OR_TEXTURE:
      return isPixel() ? "t[" + number + "]" : "a0";
    case REG_RASTOUT:
      if (reg.number == RASTOUT_FOG) {
        shader.writesFog = true;
        return "o.fog";
      }
      if (reg.number == RASTOUT_POINT_SIZE) {
        shader.writesPointSize = true;
        return "o.pointSize";
      }
      return "o.position";
    case REG_ATTROUT:
      return reg.number == 0 ? "o.diffuse" : "o.specular";
    case REG_TEXCRDOUT:
      return "o.texCoord[" + number + "]";
    default:
      return "";
  }
}

void ShaderEmitter::defineConstant(const Instruction &instruction) {
  std::string value = "float4(";
  for (int i = 0; i < 4; ++i) {
    value += (i ? ", " : "") + floatLiteral(instruction.constant[i]);
  }
  definedConstants[instruction.destination.reg.number] = value + ")";
}

std::string ShaderEmitter::definedConstantsDeclaration() const {
  std::string declaration;
  for (const auto &defined : definedConstants) {
    declaration += "    const float4 k" + std::to_string(defined.first) + " = " + defined.second + ";\n";
  }
  return declaration;
}

static std::string shifted(const std::string &value, int32_t shift) {
  switch (shift) {
    case 1:
      return "(" + value + " * 2.0)";
    case 2:
      return "(" + value + " * 4.0)";
    case 3:
      return "(" + value + " * 8.0)";
    case -1:
      return "(" + value + " * 0.5)";
    case -2:
      return "(" + value + " * 0.25)";
    case -3:
      return "(" + value + " * 0.125)";
    default:
      return value;
  }
}

std::string ShaderEmitter::finishResult(const Destination &destination, const std::string &value) const {
  std::string result = shifted(value, destination.shift);
  if (destination.saturate) {
    return "saturate(" + result + ")";
  }
  if (!isPixel()) {
    return result;
  }
  std::string range = isPixel14() ? "8.0" : "1.0";
  return "clamp(" + result + ", -" + range + ", " + range + ")";
}

std::string ShaderEmitter::commit(const Destination &destination, const std::string &value) const {
  std::string target = destinationTarget(destination.reg);
  if (target == "o.fog" || target == "o.pointSize") {
    return target + " = " + value + ".x;";
  }
  if (target == "a0") {
    return "a0.x = int(floor(" + value + ".x));";
  }
  if (destination.writeMask == FULL_WRITE_MASK) {
    return target + " = " + value + ";";
  }
  std::string mask = writeMaskSuffix(destination.writeMask);
  return target + mask + " = " + value + mask + ";";
}

void ShaderEmitter::line(const std::string &statement) {
  statements += "    " + statement + "\n";
}

const std::string &ShaderEmitter::body() const {
  return statements;
}

} // namespace D3D8Shader
