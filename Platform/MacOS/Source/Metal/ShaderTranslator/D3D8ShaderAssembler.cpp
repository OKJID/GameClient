#include "D3D8ShaderAssembler.h"
#include "D3D8ShaderOpcodes.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace D3D8Shader {

static std::string lowercase(std::string text) {
  std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return (char)std::tolower(c); });
  return text;
}

static std::string trim(const std::string &text) {
  size_t begin = text.find_first_not_of(" \t\r");
  if (begin == std::string::npos) {
    return "";
  }
  size_t end = text.find_last_not_of(" \t\r");
  return text.substr(begin, end - begin + 1);
}

static std::string withoutComment(const std::string &line) {
  size_t semicolon = line.find(';');
  size_t slashes = line.find("//");
  return line.substr(0, std::min(semicolon, slashes));
}

static std::vector<std::string> split(const std::string &text, char separator) {
  std::vector<std::string> parts;
  size_t begin = 0;
  while (true) {
    size_t end = text.find(separator, begin);
    parts.push_back(trim(text.substr(begin, end - begin)));
    if (end == std::string::npos) {
      return parts;
    }
    begin = end + 1;
  }
}

static bool componentIndex(char letter, uint32_t &lane) {
  static const char XYZW[] = "xyzw";
  static const char RGBA[] = "rgba";
  for (uint32_t i = 0; i < 4; ++i) {
    if (letter == XYZW[i] || letter == RGBA[i]) {
      lane = i;
      return true;
    }
  }
  return false;
}

static bool parseVersion(const std::string &line, Version &version) {
  std::string text = lowercase(line);
  if (text.size() != 6 || (text[2] != '.' && text[2] != '_') || text[4] != text[2]) {
    return false;
  }
  if (text.compare(0, 2, "vs") != 0 && text.compare(0, 2, "ps") != 0) {
    return false;
  }

  version.stage = text[0] == 'v' ? Stage::Vertex : Stage::Pixel;
  version.major = (uint32_t)(text[3] - '0');
  version.minor = (uint32_t)(text[5] - '0');
  return version.major == 1 && version.minor <= 4;
}

static bool parseNumberedRegister(const std::string &text, size_t prefixLength, RegisterType type, Register &reg) {
  std::string digits = text.substr(prefixLength);
  if (digits.empty() || !std::all_of(digits.begin(), digits.end(), ::isdigit)) {
    return false;
  }
  reg.type = type;
  reg.number = (uint32_t)std::atoi(digits.c_str());
  return true;
}

static bool parseRegister(const std::string &name, const Version &version, Register &reg) {
  std::string text = lowercase(name);
  if (text == "opos" || text == "ofog" || text == "opts") {
    reg.type = REG_RASTOUT;
    reg.number = text == "opos" ? RASTOUT_POSITION : text == "ofog" ? RASTOUT_FOG : RASTOUT_POINT_SIZE;
    return true;
  }
  if (text.compare(0, 2, "od") == 0) {
    return parseNumberedRegister(text, 2, REG_ATTROUT, reg);
  }
  if (text.compare(0, 2, "ot") == 0) {
    return parseNumberedRegister(text, 2, REG_TEXCRDOUT, reg);
  }

  switch (text.empty() ? '\0' : text[0]) {
    case 'r':
      return parseNumberedRegister(text, 1, REG_TEMP, reg);
    case 'v':
      return parseNumberedRegister(text, 1, REG_INPUT, reg);
    case 'c':
      return parseNumberedRegister(text, 1, REG_CONST, reg);
    case 't':
      return version.stage == Stage::Pixel && parseNumberedRegister(text, 1, REG_ADDR_OR_TEXTURE, reg);
    case 'a':
      return version.stage == Stage::Vertex && parseNumberedRegister(text, 1, REG_ADDR_OR_TEXTURE, reg);
    default:
      return false;
  }
}

static bool parseWriteMask(const std::string &letters, uint32_t &writeMask) {
  writeMask = 0;
  for (char letter : letters) {
    uint32_t lane;
    if (!componentIndex(letter, lane)) {
      return false;
    }
    writeMask |= 1u << lane;
  }
  return writeMask != 0;
}

static bool parseSwizzle(const std::string &letters, uint32_t &swizzle) {
  if (letters.empty() || letters.size() > 4) {
    return false;
  }

  swizzle = 0;
  uint32_t lane = 0;
  for (uint32_t i = 0; i < 4; ++i) {
    if (i < letters.size() && !componentIndex(letters[i], lane)) {
      return false;
    }
    swizzle |= lane << (2 * i);
  }
  return true;
}

static void splitSelector(const std::string &text, std::string &body, std::string &selector) {
  size_t closing = text.rfind(']');
  size_t dot = text.find('.', closing == std::string::npos ? 0 : closing);
  body = text.substr(0, dot);
  selector = dot == std::string::npos ? "" : lowercase(text.substr(dot + 1));
}

static std::string takeSuffix(std::string &text) {
  size_t closing = text.rfind(']');
  size_t underscore = text.find('_', closing == std::string::npos ? 0 : closing);
  if (underscore == std::string::npos) {
    return "";
  }
  std::string suffix = lowercase(text.substr(underscore + 1));
  text = text.substr(0, underscore);
  return suffix;
}

static bool suffixModifier(const std::string &suffix, SourceModifier &modifier) {
  if (suffix.empty()) {
    modifier = SRCMOD_NONE;
  } else if (suffix == "bias") {
    modifier = SRCMOD_BIAS;
  } else if (suffix == "bx2") {
    modifier = SRCMOD_SIGN;
  } else if (suffix == "x2") {
    modifier = SRCMOD_X2;
  } else if (suffix == "dz" || suffix == "db") {
    modifier = SRCMOD_DZ;
  } else if (suffix == "dw" || suffix == "da") {
    modifier = SRCMOD_DW;
  } else {
    return false;
  }
  return true;
}

static bool combineModifiers(bool negate, bool complement, SourceModifier suffix, SourceModifier &modifier) {
  if (complement) {
    modifier = SRCMOD_COMP;
    return !negate && suffix == SRCMOD_NONE;
  }
  if (!negate) {
    modifier = suffix;
    return true;
  }

  switch (suffix) {
    case SRCMOD_NONE:
      modifier = SRCMOD_NEG;
      return true;
    case SRCMOD_BIAS:
      modifier = SRCMOD_BIASNEG;
      return true;
    case SRCMOD_SIGN:
      modifier = SRCMOD_SIGNNEG;
      return true;
    case SRCMOD_X2:
      modifier = SRCMOD_X2NEG;
      return true;
    default:
      return false;
  }
}

static bool parseRelativeConstant(const std::string &body, Register &reg) {
  std::string inside = lowercase(body.substr(2, body.size() - 3));
  uint32_t offset = 0;
  bool hasAddress = false;
  for (const std::string &term : split(inside, '+')) {
    if (term == "a0.x" || term == "a0") {
      hasAddress = true;
    } else if (!term.empty() && std::all_of(term.begin(), term.end(), ::isdigit)) {
      offset = (uint32_t)std::atoi(term.c_str());
    } else {
      return false;
    }
  }
  reg.type = REG_CONST;
  reg.number = offset;
  return hasAddress;
}

static bool parseSource(std::string text, const Version &version, Source &source) {
  bool negate = text.compare(0, 1, "-") == 0;
  if (negate) {
    text = trim(text.substr(1));
  }
  bool complement = text.compare(0, 2, "1-") == 0;
  if (complement) {
    text = trim(text.substr(2));
  }

  std::string body;
  std::string selector;
  splitSelector(text, body, selector);
  std::string suffix = takeSuffix(body);
  if (suffix.empty()) {
    suffix = takeSuffix(selector);
  }

  SourceModifier suffixKind;
  if (!suffixModifier(suffix, suffixKind) || !combineModifiers(negate, complement, suffixKind, source.modifier)) {
    return false;
  }

  bool relative = lowercase(body).compare(0, 2, "c[") == 0 && body.back() == ']';
  source.relative = relative;
  bool parsed = relative ? parseRelativeConstant(body, source.reg) : parseRegister(body, version, source.reg);
  if (!parsed) {
    return false;
  }
  return selector.empty() || parseSwizzle(selector, source.swizzle);
}

static bool parseDestination(const std::string &text, const Version &version, Destination &destination) {
  std::string body;
  std::string selector;
  splitSelector(text, body, selector);
  if (!parseRegister(body, version, destination.reg)) {
    return false;
  }
  return selector.empty() || parseWriteMask(selector, destination.writeMask);
}

static bool applyInstructionModifier(const std::string &modifier, Destination &destination) {
  static const struct { const char *name; int32_t shift; } SHIFTS[] = {
    {"x2", 1}, {"x4", 2}, {"x8", 3}, {"d2", -1}, {"d4", -2}, {"d8", -3},
  };

  if (modifier == "sat") {
    destination.saturate = true;
    return true;
  }
  for (const auto &shift : SHIFTS) {
    if (modifier == shift.name) {
      destination.shift = shift.shift;
      return true;
    }
  }
  return false;
}

static bool parseMnemonic(const std::string &mnemonic, Instruction &instruction, std::string &error) {
  std::vector<std::string> parts = split(lowercase(mnemonic), '_');
  if (!opcodeFromName(parts[0], instruction.opcode)) {
    error = "unknown instruction '" + parts[0] + "'";
    return false;
  }

  size_t firstModifier = 1;
  if (instruction.opcode == OP_DCL) {
    if (parts.size() < 2 || !declarationFromUsageName(parts[1], instruction.declaration)) {
      error = "unknown declaration usage in '" + mnemonic + "'";
      return false;
    }
    firstModifier = 2;
  }

  for (size_t i = firstModifier; i < parts.size(); ++i) {
    if (!applyInstructionModifier(parts[i], instruction.destination)) {
      error = "unknown instruction modifier '_" + parts[i] + "'";
      return false;
    }
  }
  return true;
}

static bool parseConstantValues(const std::vector<std::string> &operands, Instruction &instruction) {
  for (size_t i = 0; i < 4; ++i) {
    const std::string &text = operands[i + 1];
    char *end = nullptr;
    instruction.constant[i] = std::strtof(text.c_str(), &end);
    if (end == text.c_str() || *end != '\0') {
      return false;
    }
  }
  return true;
}

static bool parseOperands(const std::vector<std::string> &operands, const OperandLayout &layout,
                          const Version &version, Instruction &instruction, std::string &error) {
  size_t expected = (layout.destination ? 1 : 0) + layout.sources + (layout.constant ? 4 : 0);
  if (operands.size() != expected) {
    error = "expected " + std::to_string(expected) + " operands, got " + std::to_string(operands.size());
    return false;
  }

  instruction.hasDestination = layout.destination;
  if (layout.destination && !parseDestination(operands[0], version, instruction.destination)) {
    error = "bad destination '" + operands[0] + "'";
    return false;
  }
  if (layout.constant) {
    if (!parseConstantValues(operands, instruction)) {
      error = "bad def values";
      return false;
    }
    return true;
  }

  for (int i = 0; i < layout.sources; ++i) {
    Source source;
    const std::string &text = operands[i + (layout.destination ? 1 : 0)];
    if (!parseSource(text, version, source)) {
      error = "bad source '" + text + "'";
      return false;
    }
    instruction.sources.push_back(source);
  }
  return true;
}

static bool parseInstruction(std::string line, const Version &version, Instruction &instruction, std::string &error) {
  instruction.coissue = line[0] == '+';
  if (instruction.coissue) {
    line = trim(line.substr(1));
  }

  size_t space = line.find_first_of(" \t");
  std::string mnemonic = line.substr(0, space);
  std::string rest = space == std::string::npos ? "" : trim(line.substr(space));
  if (!parseMnemonic(mnemonic, instruction, error)) {
    return false;
  }

  OperandLayout layout;
  operandLayout(instruction.opcode, version, layout);
  std::vector<std::string> operands = rest.empty() ? std::vector<std::string>() : split(rest, ',');
  return parseOperands(operands, layout, version, instruction, error);
}

bool assemble(const char *text, size_t length, Program &program, std::string &error) {
  std::vector<std::string> lines = split(std::string(text, length), '\n');
  bool hasVersion = false;
  int lineNumber = 0;

  for (const std::string &rawLine : lines) {
    ++lineNumber;
    std::string line = trim(withoutComment(rawLine));
    if (line.empty()) {
      continue;
    }

    if (!hasVersion) {
      if (!parseVersion(line, program.version)) {
        error = "line " + std::to_string(lineNumber) + ": expected shader version, got '" + line + "'";
        return false;
      }
      hasVersion = true;
      continue;
    }

    Instruction instruction;
    if (!parseInstruction(line, program.version, instruction, error)) {
      error = "line " + std::to_string(lineNumber) + ": " + error;
      return false;
    }
    program.instructions.push_back(instruction);
  }

  if (!hasVersion) {
    error = "empty shader";
  }
  return hasVersion;
}

} // namespace D3D8Shader
