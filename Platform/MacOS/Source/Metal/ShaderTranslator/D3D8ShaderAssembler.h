#pragma once

#include "D3D8ShaderTokens.h"

namespace D3D8Shader {

bool assemble(const char *text, size_t length, Program &program, std::string &error);

} // namespace D3D8Shader
