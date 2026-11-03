#pragma once

#include "ShaderTranslator/D3D8ShaderTranslator.h"

#include <string>

namespace MetalShaderWrappers {

constexpr unsigned int UNIFORMS_BUFFER = 1;
constexpr unsigned int FRAGMENT_UNIFORMS_BUFFER = 2;
constexpr unsigned int VERTEX_CONSTANTS_BUFFER = 4;
constexpr unsigned int PIXEL_CONSTANTS_BUFFER = 5;
constexpr unsigned int PIXEL_STATE_BUFFER = 6;

constexpr const char *VERTEX_TO_PIXEL_SHADER = "d3d8_vs_to_ps";
constexpr const char *VERTEX_TO_FIXED_FUNCTION = "d3d8_vs_to_ff";
constexpr const char *PIXEL_AFTER_VERTEX_SHADER = "d3d8_ps_after_vs";
constexpr const char *PIXEL_AFTER_FIXED_FUNCTION = "d3d8_ps_after_ff";

const char *wrapperPrelude();
std::string vertexEntries(const D3D8Shader::TranslatedShader &shader);
std::string pixelEntries(const D3D8Shader::TranslatedShader &shader);
std::string librarySource(const char *shaderInterface, const D3D8Shader::TranslatedShader &shader);

} // namespace MetalShaderWrappers
