#include "MetalShaderProgram.h"
#include "MetalShaderWrappers.h"

#import <Metal/Metal.h>

const char *macosShaderInterfaceSource();

static const size_t MAX_SHADER_TOKENS = 4096;
static const size_t MAX_DECLARATION_TOKENS = 64;
static const DWORD DECLARATION_END = 0xFFFFFFFF;
static const DWORD DECLARATION_STREAM_BIT = 0x80000000;
static const char *const SHADER_FUNCTION_NAME = "d3d8_shader";

static uint32_t declarationTypeSize(uint32_t type) {
  static const uint32_t SIZES[] = {4, 8, 12, 16, 4, 4, 4, 8};
  return type < sizeof(SIZES) / sizeof(SIZES[0]) ? SIZES[type] : 0;
}

static bool parseDeclaration(const DWORD *declaration, std::vector<VertexElement> &elements, std::string &error) {
  if (!declaration) {
    error = "vertex shader without a declaration";
    return false;
  }

  uint32_t offset = 0;
  for (size_t i = 0; i < MAX_DECLARATION_TOKENS; ++i) {
    DWORD token = declaration[i];
    if (token == DECLARATION_END) {
      return true;
    }
    if (token & DECLARATION_STREAM_BIT) {
      if ((token & 0xFFFF) != 0) {
        error = "vertex stream " + std::to_string(token & 0xFFFF) + " is not supported, only stream 0";
        return false;
      }
      offset = 0;
      continue;
    }

    uint32_t reg = token & 0xFFFF;
    uint32_t type = (token >> 16) & 0xFFFF;
    uint32_t size = declarationTypeSize(type);
    if (reg >= D3D8Shader::MAX_VERTEX_INPUTS || size == 0) {
      error = "unsupported declaration token " + std::to_string(token);
      return false;
    }
    elements.push_back({reg, type, offset});
    offset += size;
  }

  error = "declaration without an end token";
  return false;
}

static bool translateTokens(const DWORD *function, D3D8Shader::Stage stage, D3D8Shader::TranslatedShader &shader,
                            std::string &error) {
  if (!function) {
    error = "no shader function";
    return false;
  }

  D3D8Shader::Program program;
  if (!D3D8Shader::decode((const uint32_t *)function, MAX_SHADER_TOKENS, program, error)) {
    return false;
  }
  if (program.version.stage != stage) {
    error = stage == D3D8Shader::Stage::Vertex ? "not a vertex shader" : "not a pixel shader";
    return false;
  }
  return D3D8Shader::translate(program, SHADER_FUNCTION_NAME, shader, error);
}

static bool compileLibrary(void *device, MetalShaderProgram &program, const char *shaderPartnerEntry,
                           const char *fixedFunctionPartnerEntry, std::string &error) {
  std::string source = MetalShaderWrappers::librarySource(macosShaderInterfaceSource(), program.shader);
  MTLCompileOptions *options = [[MTLCompileOptions alloc] init];
  options.languageVersion = MTLLanguageVersion3_0;

  NSError *compileError = nil;
  id<MTLLibrary> library = [(__bridge id<MTLDevice>)device newLibraryWithSource:@(source.c_str())
                                                                         options:options
                                                                           error:&compileError];
  if (!library) {
    error = compileError ? [[compileError localizedDescription] UTF8String] : "Metal compiler gave no reason";
    return false;
  }

  id<MTLFunction> withShaderPartner = [library newFunctionWithName:@(shaderPartnerEntry)];
  id<MTLFunction> withFixedFunctionPartner = [library newFunctionWithName:@(fixedFunctionPartnerEntry)];
  if (!withShaderPartner || !withFixedFunctionPartner) {
    error = "translated library lacks its entry points";
    return false;
  }

  program.library = (__bridge_retained void *)library;
  program.functionWithShaderPartner = (__bridge_retained void *)withShaderPartner;
  program.functionWithFixedFunctionPartner = (__bridge_retained void *)withFixedFunctionPartner;
  return true;
}

static void releaseObject(void *&object) {
  if (object) {
    CFRelease(object);
    object = nullptr;
  }
}

MetalShaderProgram::~MetalShaderProgram() {
  releaseObject(functionWithShaderPartner);
  releaseObject(functionWithFixedFunctionPartner);
  releaseObject(library);
}

MetalShaderProgram *MetalShaderProgram::createVertexShader(void *device, const DWORD *declaration, const DWORD *function,
                                                           std::string &error) {
  MetalShaderProgram *program = new MetalShaderProgram();
  bool created = parseDeclaration(declaration, program->elements, error)
    && translateTokens(function, D3D8Shader::Stage::Vertex, program->shader, error)
    && compileLibrary(device, *program, MetalShaderWrappers::VERTEX_TO_PIXEL_SHADER,
                      MetalShaderWrappers::VERTEX_TO_FIXED_FUNCTION, error);
  if (!created) {
    delete program;
    return nullptr;
  }
  return program;
}

MetalShaderProgram *MetalShaderProgram::createPixelShader(void *device, const DWORD *function, std::string &error) {
  MetalShaderProgram *program = new MetalShaderProgram();
  bool created = translateTokens(function, D3D8Shader::Stage::Pixel, program->shader, error)
    && compileLibrary(device, *program, MetalShaderWrappers::PIXEL_AFTER_VERTEX_SHADER,
                      MetalShaderWrappers::PIXEL_AFTER_FIXED_FUNCTION, error);
  if (!created) {
    delete program;
    return nullptr;
  }
  return program;
}
