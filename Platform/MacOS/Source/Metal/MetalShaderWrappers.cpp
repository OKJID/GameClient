#include "MetalShaderWrappers.h"

namespace MetalShaderWrappers {

using D3D8Shader::TranslatedShader;

const char *wrapperPrelude() {
  return R"MSL(
struct D3D8VertexVaryings {
    float4 position [[position]];
    float4 diffuse;
    float4 specular;
    float4 texCoord0;
    float4 texCoord1;
    float4 texCoord2;
    float4 texCoord3;
    float4 texCoord4;
    float4 texCoord5;
    float4 texCoord6;
    float4 texCoord7;
    float fog;
    float pointSize [[point_size]];
    float clipDistance [[clip_distance]] [6];
};

struct D3D8PixelVaryings {
    float4 position [[position]];
    float4 diffuse;
    float4 specular;
    float4 texCoord0;
    float4 texCoord1;
    float4 texCoord2;
    float4 texCoord3;
    float4 texCoord4;
    float4 texCoord5;
    float4 texCoord6;
    float4 texCoord7;
    float fog;
};

struct D3D8PixelColorAndDepth {
    float4 color [[color(0)]];
    float depth [[depth(any)]];
};

inline float4 d3d8_finishPixel(D3D8PixelShaderOutput o, float fog, constant FragmentUniforms &fragUniforms) {
    if (o.killed) {
        discard_fragment();
    }
    float4 color = saturate(o.color);
    if (fragUniforms.alphaTestEnable != 0 && !alphaTestPass(fragUniforms.alphaFunc, color.a, fragUniforms.alphaRef)) {
        discard_fragment();
    }
    if (fragUniforms.fogMode != 0) {
        color.rgb = mix(fragUniforms.fogColor.rgb, color.rgb, fog);
    }
    return color;
}

)MSL";
}

static std::string buffer(unsigned int index) {
  return std::to_string(index);
}

static std::string vertexInputs(const TranslatedShader &shader) {
  std::string declaration = "struct D3D8VertexInputs {\n";
  for (uint32_t reg = 0; reg < D3D8Shader::MAX_VERTEX_INPUTS; ++reg) {
    if (shader.inputRegisterMask & (1u << reg)) {
      declaration += "    float4 v" + std::to_string(reg) + " [[attribute(" + std::to_string(reg) + ")]];\n";
    }
  }
  return declaration + "};\n\n";
}

static std::string runVertexShader(const TranslatedShader &shader) {
  bool hasInputs = shader.inputRegisterMask != 0;
  std::string text = hasInputs ? vertexInputs(shader) : "";
  text += "static D3D8VertexShaderOutput d3d8_runVertexShader(";
  text += hasInputs ? "D3D8VertexInputs in, " : "";
  text += "constant float4 *c) {\n"
          "    float4 v[16];\n"
          "    for (int i = 0; i < 16; ++i) {\n"
          "        v[i] = float4(0.0, 0.0, 0.0, 1.0);\n"
          "    }\n";
  for (uint32_t reg = 0; reg < D3D8Shader::MAX_VERTEX_INPUTS; ++reg) {
    if (shader.inputRegisterMask & (1u << reg)) {
      std::string number = std::to_string(reg);
      text += "    v[" + number + "] = in.v" + number + ";\n";
    }
  }
  return text + "    return " + shader.functionName + "(v, c);\n}\n\n";
}

static std::string vertexParameters(const TranslatedShader &shader) {
  std::string parameters = shader.inputRegisterMask != 0 ? "D3D8VertexInputs in [[stage_in]], " : "";
  return parameters + "constant Uniforms &uniforms [[buffer(" + buffer(UNIFORMS_BUFFER) + ")]], constant float4 *c [[buffer("
    + buffer(VERTEX_CONSTANTS_BUFFER) + ")]]";
}

static std::string vertexShaderCall(const TranslatedShader &shader) {
  return std::string("    D3D8VertexShaderOutput o = d3d8_runVertexShader(") + (shader.inputRegisterMask != 0 ? "in, " : "")
    + "c);\n";
}

static std::string pointSize(const TranslatedShader &shader) {
  return shader.writesPointSize ? "max(o.pointSize, 1.0)" : "max(uniforms.pointSize, 1.0)";
}

static std::string vertexToPixelShader(const TranslatedShader &shader) {
  std::string text = std::string("vertex D3D8VertexVaryings ") + VERTEX_TO_PIXEL_SHADER + "(" + vertexParameters(shader) + ") {\n"
    + vertexShaderCall(shader) +
    "    D3D8VertexVaryings out;\n"
    "    out.position = o.position;\n"
    "    out.diffuse = saturate(o.diffuse);\n"
    "    out.specular = saturate(o.specular);\n";
  for (int i = 0; i < 8; ++i) {
    std::string n = std::to_string(i);
    text += "    out.texCoord" + n + " = o.texCoord[" + n + "];\n";
  }
  return text +
    "    out.fog = saturate(o.fog);\n"
    "    out.pointSize = " + pointSize(shader) + ";\n"
    "    for (int i = 0; i < 6; ++i) {\n"
    "        out.clipDistance[i] = (uniforms.clipPlaneEnable & (1u << i)) != 0u ? dot(o.position, uniforms.clipPlanes[i]) : 1.0;\n"
    "    }\n"
    "    return out;\n"
    "}\n\n";
}

static std::string vertexToFixedFunction(const TranslatedShader &shader) {
  std::string text = std::string("vertex VertexOut ") + VERTEX_TO_FIXED_FUNCTION + "(" + vertexParameters(shader) + ") {\n"
    + vertexShaderCall(shader) +
    "    VertexOut out;\n"
    "    out.position = o.position;\n"
    "    out.color = saturate(o.diffuse);\n"
    "    out.specularColor = saturate(o.specular);\n";
  for (int i = 0; i < 8; ++i) {
    std::string n = std::to_string(i);
    text += "    out.texCoord" + n + " = o.texCoord[" + n + "];\n";
  }
  return text +
    "    out.fogFactor = saturate(o.fog);\n"
    "    out.camPosX = 0.0;\n"
    "    out.camPosY = 0.0;\n"
    "    out.camPosZ = 0.0;\n"
    "    out.camNormalX = 0.0;\n"
    "    out.camNormalY = 0.0;\n"
    "    out.camNormalZ = 0.0;\n"
    "    out.pointSize = " + pointSize(shader) + ";\n"
    "    applyClipPlanes(uniforms, o.position, out);\n"
    "    return out;\n"
    "}\n\n";
}

std::string vertexEntries(const TranslatedShader &shader) {
  return runVertexShader(shader) + vertexToPixelShader(shader) + vertexToFixedFunction(shader);
}

static std::string textureParameters() {
  return "        texture2d<float> tex0 [[texture(0)]], texture2d<float> tex1 [[texture(1)]],\n"
         "        texture2d<float> tex2 [[texture(2)]], texture2d<float> tex3 [[texture(3)]],\n"
         "        texturecube<float> cube0 [[texture(8)]], texturecube<float> cube1 [[texture(9)]],\n"
         "        texturecube<float> cube2 [[texture(10)]], texturecube<float> cube3 [[texture(11)]],\n"
         "        sampler s0 [[sampler(0)]], sampler s1 [[sampler(1)]], sampler s2 [[sampler(2)]], sampler s3 [[sampler(3)]]";
}

static std::string pixelShaderParameters() {
  return "        constant FragmentUniforms &fragUniforms [[buffer(" + buffer(FRAGMENT_UNIFORMS_BUFFER) + ")]],\n"
    "        constant float4 *c [[buffer(" + buffer(PIXEL_CONSTANTS_BUFFER) + ")]],\n"
    "        constant D3D8PixelShaderState &state [[buffer(" + buffer(PIXEL_STATE_BUFFER) + ")]],\n"
    + textureParameters();
}

static std::string pixelShaderCall(const TranslatedShader &shader) {
  return "    D3D8PixelShaderOutput o = " + shader.functionName + "(psIn, c, state, tex0, tex1, tex2, tex3,\n"
    "        cube0, cube1, cube2, cube3, s0, s1, s2, s3);\n";
}

static std::string pixelReturnType(const TranslatedShader &shader) {
  return shader.writesDepth ? "D3D8PixelColorAndDepth" : "float4";
}

static std::string pixelReturn(const TranslatedShader &shader, const std::string &fog) {
  if (!shader.writesDepth) {
    return "    return d3d8_finishPixel(o, " + fog + ", fragUniforms);\n";
  }
  return "    D3D8PixelColorAndDepth out;\n"
         "    out.color = d3d8_finishPixel(o, " + fog + ", fragUniforms);\n"
         "    out.depth = o.depth;\n"
         "    return out;\n";
}

static std::string pixelAfterVertexShader(const TranslatedShader &shader) {
  std::string text = "fragment " + pixelReturnType(shader) + " " + PIXEL_AFTER_VERTEX_SHADER
    + "(D3D8PixelVaryings in [[stage_in]],\n" + pixelShaderParameters() + ") {\n"
    "    D3D8PixelShaderInput psIn;\n"
    "    psIn.diffuse = in.diffuse;\n"
    "    psIn.specular = in.specular;\n";
  for (int i = 0; i < 8; ++i) {
    std::string n = std::to_string(i);
    text += "    psIn.texCoord[" + n + "] = in.texCoord" + n + ";\n";
  }
  return text + pixelShaderCall(shader) + pixelReturn(shader, "in.fog") + "}\n\n";
}

static std::string pixelAfterFixedFunction(const TranslatedShader &shader) {
  return "fragment " + pixelReturnType(shader) + " " + PIXEL_AFTER_FIXED_FUNCTION + "(FragmentIn stageIn [[stage_in]],\n"
    "        constant Uniforms &uniforms [[buffer(" + buffer(UNIFORMS_BUFFER) + ")]],\n"
    + pixelShaderParameters() + ",\n"
    "        float2 pointCoord [[point_coord]]) {\n"
    "    FragmentIn in = stageIn;\n"
    "    if (uniforms.pointSpriteEnable != 0) {\n"
    "        applyPointSpriteTexCoords(in, pointCoord);\n"
    "    }\n"
    "    D3D8PixelShaderInput psIn;\n"
    "    psIn.diffuse = in.color;\n"
    "    psIn.specular = in.specularColor;\n"
    "    for (uint i = 0; i < 8; ++i) {\n"
    "        bool clipped;\n"
    "        psIn.texCoord[i] = computeTexCoord(uniforms, in, fragUniforms.texCoordIndex[i], i, clipped);\n"
    "    }\n"
    + pixelShaderCall(shader) + pixelReturn(shader, "in.fogFactor") + "}\n\n";
}

std::string pixelEntries(const TranslatedShader &shader) {
  return pixelAfterVertexShader(shader) + pixelAfterFixedFunction(shader);
}

std::string librarySource(const char *shaderInterface, const TranslatedShader &shader) {
  std::string entries = shader.stage == D3D8Shader::Stage::Vertex ? vertexEntries(shader) : pixelEntries(shader);
  return std::string(shaderInterface) + "\n" + D3D8Shader::shaderPrelude() + wrapperPrelude() + shader.source + "\n" + entries;
}

} // namespace MetalShaderWrappers
