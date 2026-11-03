#include "D3D8ShaderAssembler.h"
#include "D3D8ShaderTokens.h"
#include "D3D8ShaderTranslator.h"
#include "MetalShaderWrappers.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

using namespace D3D8Shader;

static bool readFile(const std::string &path, std::string &content) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return false;
  }
  content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
  return true;
}

static bool endsWith(const std::string &text, const std::string &suffix) {
  return text.size() >= suffix.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

static bool loadProgram(const std::string &path, Program &program, std::vector<uint32_t> &original, std::string &error) {
  std::string content;
  if (!readFile(path, content)) {
    error = "cannot read";
    return false;
  }

  if (endsWith(path, ".asm")) {
    if (!assemble(content.data(), content.size(), program, error)) {
      return false;
    }
    original = encode(program);
    return true;
  }

  std::vector<uint32_t> tokens(content.size() / 4);
  memcpy(tokens.data(), content.data(), tokens.size() * 4);
  original = withoutComments(tokens.data(), tokens.size());
  return decode(tokens.data(), tokens.size(), program, error);
}

static bool roundTrip(const std::string &path) {
  Program program;
  std::vector<uint32_t> original;
  std::string error;
  if (!loadProgram(path, program, original, error)) {
    printf("FAIL %s: %s\n", path.c_str(), error.c_str());
    return false;
  }

  std::string text = disassemble(program);
  Program reassembled;
  if (!assemble(text.data(), text.size(), reassembled, error)) {
    printf("FAIL %s: reassemble: %s\n%s", path.c_str(), error.c_str(), text.c_str());
    return false;
  }

  std::vector<uint32_t> tokens = encode(reassembled);
  if (tokens != original) {
    printf("FAIL %s: tokens differ after round trip\n%s", path.c_str(), text.c_str());
    for (size_t i = 0; i < std::max(tokens.size(), original.size()); ++i) {
      uint32_t want = i < original.size() ? original[i] : 0;
      uint32_t got = i < tokens.size() ? tokens[i] : 0;
      printf("  %3zu %08x %08x%s\n", i, want, got, want == got ? "" : "  <--");
    }
    return false;
  }

  printf("ok   %s (%zu instructions)\n", path.c_str(), program.instructions.size());
  return true;
}

static bool printDisassembly(const std::string &path) {
  Program program;
  std::vector<uint32_t> original;
  std::string error;
  if (!loadProgram(path, program, original, error)) {
    printf("FAIL %s: %s\n", path.c_str(), error.c_str());
    return false;
  }
  printf("; %s\n%s\n", path.c_str(), disassemble(program).c_str());
  return true;
}

static std::string identifierFor(const std::string &path) {
  std::string name = path.substr(path.find_last_of('/') + 1);
  std::string identifier = "shader_";
  for (char c : name) {
    identifier += isalnum((unsigned char)c) ? c : '_';
  }
  return identifier;
}

static bool translateAll(const std::string &outputDir, const std::string &interfacePath, int count, char **paths) {
  std::string shaderInterface;
  if (!readFile(interfacePath, shaderInterface)) {
    printf("FAIL cannot read %s\n", interfacePath.c_str());
    return false;
  }

  int failures = 0;
  for (int i = 0; i < count; ++i) {
    Program program;
    std::vector<uint32_t> original;
    std::string error;
    TranslatedShader shader;
    if (!loadProgram(paths[i], program, original, error) || !translate(program, identifierFor(paths[i]), shader, error)) {
      printf("FAIL %s: %s\n", paths[i], error.c_str());
      ++failures;
      continue;
    }

    std::ofstream(outputDir + "/" + shader.functionName + ".metal")
      << "// " << paths[i] << "\n" << MetalShaderWrappers::librarySource(shaderInterface.c_str(), shader);
    printf("ok   %s -> %s.metal\n", paths[i], shader.functionName.c_str());
  }

  printf("%d of %d failed to translate\n", failures, count);
  return failures == 0;
}

int main(int argc, char **argv) {
  if (argc >= 5 && strcmp(argv[1], "translate") == 0) {
    return translateAll(argv[2], argv[3], argc - 4, argv + 4) ? 0 : 1;
  }

  bool (*command)(const std::string &) = nullptr;
  if (argc >= 3 && strcmp(argv[1], "roundtrip") == 0) {
    command = roundTrip;
  } else if (argc >= 3 && strcmp(argv[1], "disasm") == 0) {
    command = printDisassembly;
  } else {
    fprintf(stderr, "usage: %s roundtrip|disasm <shader.pso|.vso|.asm>...\n"
                    "       %s translate <out_dir> <MacOSShaderInterface.h> <shader.pso|.vso|.asm>...\n", argv[0], argv[0]);
    return 2;
  }

  int failures = 0;
  for (int i = 2; i < argc; ++i) {
    failures += command(argv[i]) ? 0 : 1;
  }
  printf("%d of %d failed\n", failures, argc - 2);
  return failures == 0 ? 0 : 1;
}
