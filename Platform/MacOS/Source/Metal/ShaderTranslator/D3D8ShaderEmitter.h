#pragma once

#include "D3D8ShaderTranslator.h"

#include <map>

namespace D3D8Shader {

class ShaderEmitter {
public:
  const Program &program;
  TranslatedShader &shader;

  ShaderEmitter(const Program &program, TranslatedShader &shader);

  bool isPixel() const;
  bool isPixel14() const;

  bool checkRegister(const Register &reg, bool destination, std::string &error) const;
  std::string registerValue(const Register &reg, bool relative) const;
  std::string registerRow(const Source &source, uint32_t row) const;
  std::string sourceValue(const Source &source) const;
  std::string destinationTarget(const Register &reg) const;

  void defineConstant(const Instruction &instruction);
  std::string definedConstantsDeclaration() const;

  std::string finishResult(const Destination &destination, const std::string &value) const;
  std::string commit(const Destination &destination, const std::string &value) const;

  void line(const std::string &statement);
  const std::string &body() const;

private:
  std::map<uint32_t, std::string> definedConstants;
  std::string statements;

  std::string constantValue(uint32_t number, bool relative) const;
  std::string pixelRegisterValue(const Register &reg) const;
  std::string vertexRegisterValue(const Register &reg, bool relative) const;
};

std::string swizzleSuffix(uint32_t swizzle);
std::string writeMaskSuffix(uint32_t writeMask);
std::string floatLiteral(float value);

} // namespace D3D8Shader
