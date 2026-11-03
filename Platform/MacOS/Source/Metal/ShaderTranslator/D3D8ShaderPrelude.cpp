#include "D3D8ShaderTranslator.h"

namespace D3D8Shader {

const char *shaderPrelude() {
  return R"MSL(#include <metal_stdlib>
using namespace metal;

struct D3D8VertexShaderOutput {
    float4 position;
    float4 diffuse;
    float4 specular;
    float4 texCoord[8];
    float fog;
    float pointSize;
};

struct D3D8PixelShaderInput {
    float4 diffuse;
    float4 specular;
    float4 texCoord[8];
};

struct D3D8PixelShaderState {
    float4 bumpEnvMatrix[8];
    float4 bumpEnvLuminance[8];
    uint projectionDivisor[8];
    uint textureFormat[8];
    uint cubeStageMask;
    uint boundStageMask;
    uint _pad[2];
};

struct D3D8PixelShaderOutput {
    float4 color;
    float depth;
    bool killed;
};

inline float4 d3d8_vsConstant(constant float4 *c, int index) {
    return index >= 0 && index < 96 ? c[index] : float4(0.0);
}

inline float d3d8_rcp(float x) {
    if (x == 1.0) {
        return 1.0;
    }
    return x == 0.0 ? INFINITY : 1.0 / x;
}

inline float d3d8_rsq(float x) {
    float magnitude = fabs(x);
    if (magnitude == 1.0) {
        return 1.0;
    }
    return magnitude == 0.0 ? INFINITY : rsqrt(magnitude);
}

inline float d3d8_log(float x) {
    float magnitude = fabs(x);
    return magnitude == 0.0 ? -INFINITY : log2(magnitude);
}

inline float4 d3d8_expp(float x) {
    float whole = floor(x);
    return float4(exp2(whole), x - whole, exp2(x), 1.0);
}

inline float4 d3d8_logp(float x) {
    float magnitude = fabs(x);
    if (magnitude == 0.0) {
        return float4(-INFINITY, 1.0, -INFINITY, 1.0);
    }
    float exponent = floor(log2(magnitude));
    return float4(exponent, magnitude / exp2(exponent), log2(magnitude), 1.0);
}

inline float4 d3d8_lit(float4 s) {
    float4 result = float4(1.0, 0.0, 0.0, 1.0);
    if (s.x > 0.0) {
        result.y = s.x;
        result.z = s.y > 0.0 ? pow(s.y, clamp(s.w, -128.0, 128.0)) : 0.0;
    }
    return result;
}

inline float4 d3d8_dst(float4 a, float4 b) {
    return float4(1.0, a.y * b.y, a.z, b.w);
}

inline float4 d3d8_slt(float4 a, float4 b) {
    return select(float4(0.0), float4(1.0), a < b);
}

inline float4 d3d8_sge(float4 a, float4 b) {
    return select(float4(0.0), float4(1.0), a >= b);
}

inline float3 d3d8_projected(float4 coord, uint divisor) {
    if (divisor == 0u || coord[divisor] == 0.0) {
        return coord.xyz;
    }
    return coord.xyz / coord[divisor];
}

inline float4 d3d8_unpackFormat(float4 texel, uint format) {
    if (format == 1u) {
        return float4(texel.r, texel.r, texel.r, 1.0);
    }
    if (format == 2u) {
        return float4(texel.r, texel.r, texel.r, texel.g);
    }
    if (format == 3u && all(texel.rgb == 0.0)) {
        texel.a = 0.0;
    }
    return texel;
}

inline float4 d3d8_sample(texture2d<float> flat, texturecube<float> cube, sampler s, constant D3D8PixelShaderState &state,
                          uint stage, float3 coord) {
    if (((state.boundStageMask >> stage) & 1u) == 0u) {
        return float4(0.0, 0.0, 0.0, 1.0);
    }
    float4 texel = ((state.cubeStageMask >> stage) & 1u) != 0u ? cube.sample(s, coord) : flat.sample(s, coord.xy);
    return d3d8_unpackFormat(texel, state.textureFormat[stage]);
}

inline float3 d3d8_reflect(float3 normal, float3 eye) {
    float lengthSquared = dot(normal, normal);
    if (lengthSquared == 0.0) {
        return -eye;
    }
    return 2.0 * normal * dot(normal, eye) / lengthSquared - eye;
}

)MSL";
}

} // namespace D3D8Shader
