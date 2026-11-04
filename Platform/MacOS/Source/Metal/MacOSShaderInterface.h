#pragma once

#include <metal_stdlib>
using namespace metal;

// ─────────────────────────────────────────────────────
//  Vertex Uniforms (buffer 1)
// ─────────────────────────────────────────────────────
struct Uniforms {
    float4x4 world;
    float4x4 view;
    float4x4 projection;
    float4x4 texMatrix[8];  // D3DTS_TEXTURE0..7 — UV transform matrices
    float2 screenSize;
    int useProjection;      // 0=passthrough, 1=3D, 2=2D screen space
    uint shaderSettings;    // legacy bitfield (texturing bit etc.)
    uint texTransformFlags[8]; // D3DTSS_TEXTURETRANSFORMFLAGS per stage (0=disabled, 2=COUNT2)
    uint texCoordSize[8];      // components of each FVF texcoord set, 0 when the set is absent
    float4 clipPlanes[6];
    uint clipPlaneEnable;
    // Point sprites (D3DRS_POINTSPRITEENABLE / POINTSCALE*).
    // Scalars only — uint3/float3 carry 16-byte alignment that would desync
    // this tail from MetalUniforms on the C++ side.
    uint pointSpriteEnable;
    uint pointScaleEnable;
    float pointSize;
    float pointSizeMin;
    float pointSizeMax;
    float pointScaleA;
    float pointScaleB;
    float pointScaleC;
    uint vertexProgramActive; // texcoords come from a DX8 vertex shader
    uint _pad1;
    uint _pad2;
};

// ─────────────────────────────────────────────────────
//  Stage 7: Fragment Uniforms (buffer 2)
//  Mirrors FragmentUniforms in MetalDevice8.mm
// ─────────────────────────────────────────────────────

// D3DTOP enum values (must match d3d8_stub.h)
#define D3DTOP_DISABLE           1
#define D3DTOP_SELECTARG1        2
#define D3DTOP_SELECTARG2        3
#define D3DTOP_MODULATE          4
#define D3DTOP_MODULATE2X        5
#define D3DTOP_MODULATE4X        6
#define D3DTOP_ADD               7
#define D3DTOP_ADDSIGNED         8
#define D3DTOP_ADDSIGNED2X       9
#define D3DTOP_SUBTRACT         10
#define D3DTOP_ADDSMOOTH        11
#define D3DTOP_BLENDDIFFUSEALPHA  12
#define D3DTOP_BLENDTEXTUREALPHA  13
#define D3DTOP_BLENDFACTORALPHA   14
#define D3DTOP_BLENDTEXTUREALPHAPM 15
#define D3DTOP_BLENDCURRENTALPHA  16
#define D3DTOP_PREMODULATE       17
#define D3DTOP_MODULATEALPHA_ADDCOLOR   18
#define D3DTOP_MODULATECOLOR_ADDALPHA   19
#define D3DTOP_MODULATEINVALPHA_ADDCOLOR 20
#define D3DTOP_MODULATEINVCOLOR_ADDALPHA 21
#define D3DTOP_BUMPENVMAP        22
#define D3DTOP_BUMPENVMAPLUMINANCE 23
#define D3DTOP_DOTPRODUCT3       24
#define D3DTOP_MULTIPLYADD       25
#define D3DTOP_LERP              26

// D3DTA source selectors (low 4 bits)
#define D3DTA_DIFFUSE  0
#define D3DTA_CURRENT  1
#define D3DTA_TEXTURE  2
#define D3DTA_TFACTOR  3
#define D3DTA_SPECULAR 4
#define D3DTA_TEMP     5
// D3DTA modifiers (high bits)
#define D3DTA_COMPLEMENT     0x10
#define D3DTA_ALPHAREPLICATE 0x20

// D3DCMP enum values
#define D3DCMP_NEVER        1
#define D3DCMP_LESS         2
#define D3DCMP_EQUAL        3
#define D3DCMP_LESSEQUAL    4
#define D3DCMP_GREATER      5
#define D3DCMP_NOTEQUAL     6
#define D3DCMP_GREATEREQUAL 7
#define D3DCMP_ALWAYS       8

// texFormatType values
#define TEXFORMAT_DEFAULT         0
#define TEXFORMAT_LUMINANCE       1  // L8/P8: (r, r, r, 1)
#define TEXFORMAT_LUMINANCE_ALPHA 2  // A8L8/A4L4/A8P8: (r, r, r, g)
#define TEXFORMAT_DXT1            3  // BC1: black texels are transparent
#define TEXFORMAT_NO_ALPHA        4  // X8R8G8B8 and other alpha-less formats: alpha reads 1
#define TEXFORMAT_BUMP_DUDV       5  // V8U8: (du, dv, 1, 1)

// D3DTEXTURETRANSFORMFLAGS
#define D3DTTFF_PROJECTED 256

struct TextureStageConfig {
    uint colorOp;     // D3DTOP enum
    uint colorArg1;   // D3DTA enum
    uint colorArg2;   // D3DTA enum
    uint alphaOp;     // D3DTOP enum
    uint alphaArg1;   // D3DTA enum
    uint alphaArg2;   // D3DTA enum
    uint colorArg0;   // D3DTA enum (3rd arg for MULTIPLYADD, LERP)
    uint alphaArg0;   // D3DTA enum (3rd arg for MULTIPLYADD, LERP)
    uint resultArg;   // D3DTA_CURRENT or D3DTA_TEMP
    float bumpEnvLScale;  // D3DTSS_BUMPENVLSCALE
    float bumpEnvLOffset; // D3DTSS_BUMPENVLOFFSET
    float lodBias;        // D3DTSS_MIPMAPLODBIAS
    float4 bumpEnvMatrix; // D3DTSS_BUMPENVMAT00, 01, 10, 11
};

struct FragmentUniforms {
    TextureStageConfig stages[8];
    float4 textureFactor;   // D3DRS_TEXTUREFACTOR (ARGB as float4)
    float4 fogColor;
    float  fogStart;
    float  fogEnd;
    float  fogDensity;
    uint   fogMode;
    uint   alphaTestEnable;
    uint   alphaFunc;        // D3DCMP enum
    float  alphaRef;         // normalized 0..1
    uint   hasTexture[8];
    uint   specularEnable;
    uint   texCoordIndex[8]; // D3DTSS_TEXCOORDINDEX: which UV set each stage uses
    uint   texFormatType[8]; // 0=Default, 1=Luminance(L8/P8), 2=Luminance+Alpha(A4L4/A8L8), 3=DXT1(BC1), 4=NoAlpha(X8R8G8B8), 5=V8U8
    uint   blendEnabled;     // D3DRS_ALPHABLENDENABLE — used by black discard guard
};

struct VertexOut {
    float4 position [[position]];
    float4 color;
    float4 specularColor;
    float4 texCoord0;
    float4 texCoord1;
    float4 texCoord2;
    float4 texCoord3;
    float4 texCoord4;
    float4 texCoord5;
    float4 texCoord6;
    float4 texCoord7;
    float fogFactor;
    // Camera-space position for TCI_CAMERASPACEPOSITION.
    // Using 3 separate floats instead of float3 to avoid 16-byte alignment
    // padding that corrupts fogFactor interpolation in Metal rasterizer.
    float camPosX;
    float camPosY;
    float camPosZ;
    float camNormalX;
    float camNormalY;
    float camNormalZ;
    float pointSize [[point_size]];
    float clipDistance [[clip_distance]] [6];
};

struct FragmentIn {
    float4 position [[position]];
    float4 color;
    float4 specularColor;
    float4 texCoord0;
    float4 texCoord1;
    float4 texCoord2;
    float4 texCoord3;
    float4 texCoord4;
    float4 texCoord5;
    float4 texCoord6;
    float4 texCoord7;
    float fogFactor;
    float camPosX;
    float camPosY;
    float camPosZ;
    float camNormalX;
    float camNormalY;
    float camNormalZ;
};

static void applyClipPlanes(constant Uniforms &uniforms, float4 wPos, thread VertexOut &vOut) {
    if (uniforms.clipPlaneEnable != 0) {
        for (int i = 0; i < 6; ++i) {
            if ((uniforms.clipPlaneEnable & (1 << i)) != 0) {
                vOut.clipDistance[i] = dot(wPos, uniforms.clipPlanes[i]);
            } else {
                vOut.clipDistance[i] = 1.0;
            }
        }
    } else {
        for (int i = 0; i < 6; ++i) vOut.clipDistance[i] = 1.0;
    }
}

// ─────────────────────────────────────────────────────
//  Alpha test comparison
// ─────────────────────────────────────────────────────
bool alphaTestPass(uint func, float alphaVal, float ref) {
    switch (func) {
        case D3DCMP_NEVER:        return false;
        case D3DCMP_LESS:         return alphaVal <  ref;
        case D3DCMP_EQUAL:        return abs(alphaVal - ref) < 0.004;
        case D3DCMP_LESSEQUAL:    return alphaVal <= ref;
        case D3DCMP_GREATER:      return alphaVal >  ref;
        case D3DCMP_NOTEQUAL:     return abs(alphaVal - ref) >= 0.004;
        case D3DCMP_GREATEREQUAL: return alphaVal >= ref;
        case D3DCMP_ALWAYS:       return true;
        default:                  return true;
    }
}

// D3DTSS_TCI_* >> 16, numbered as in Platform/MacOS/Include/d3d8.h
#define TCI_PASSTHRU                    0
#define TCI_CAMERASPACEPOSITION         1
#define TCI_CAMERASPACENORMAL           2
#define TCI_CAMERASPACEREFLECTIONVECTOR 3

static float4 vertexTexCoord(thread FragmentIn &in, uint index) {
    switch (index) {
        case 0:  return in.texCoord0;
        case 1:  return in.texCoord1;
        case 2:  return in.texCoord2;
        case 3:  return in.texCoord3;
        case 4:  return in.texCoord4;
        case 5:  return in.texCoord5;
        case 6:  return in.texCoord6;
        default: return in.texCoord7;
    }
}

static void applyPointSpriteTexCoords(thread FragmentIn &in, float2 pointCoord) {
    float4 spriteCoord = float4(pointCoord, 0.0, 1.0);
    in.texCoord0 = spriteCoord;
    in.texCoord1 = spriteCoord;
    in.texCoord2 = spriteCoord;
    in.texCoord3 = spriteCoord;
    in.texCoord4 = spriteCoord;
    in.texCoord5 = spriteCoord;
    in.texCoord6 = spriteCoord;
    in.texCoord7 = spriteCoord;
}

static uint texGenMode(constant Uniforms &uniforms, uint tci) {
    uint mode = (tci >> 16) & 0x3;
    if (mode == TCI_CAMERASPACEPOSITION && uniforms.useProjection != 1) {
        return TCI_PASSTHRU;
    }
    return mode;
}

static float4 generatedTexCoord(thread FragmentIn &in, uint mode) {
    float3 position = float3(in.camPosX, in.camPosY, in.camPosZ);
    float3 normal = float3(in.camNormalX, in.camNormalY, in.camNormalZ);
    if (mode == TCI_CAMERASPACEPOSITION) {
        return float4(position, 1.0);
    }
    if (mode == TCI_CAMERASPACENORMAL) {
        return float4(normal, 1.0);
    }
    float3 view = normalize(position);
    return float4(view - 2.0 * dot(view, normal) * normal, 1.0);
}

static float4 texCoordAsTransformInput(float4 tc, uint size) {
    if (size == 1) {
        return float4(tc.x, 1.0, 0.0, 0.0);
    }
    if (size == 2) {
        return float4(tc.xy, 1.0, 0.0);
    }
    return tc;
}

static float4 truncatedTexCoord(float4 tc, uint count) {
    return select(float4(0.0, 0.0, 0.0, 1.0), tc, uint4(0, 1, 2, 3) < uint4(count));
}

static float4 projectedTexCoord(float4 tc, uint flags, thread bool &clipped) {
    uint count = flags & 0xFF;
    if ((flags & D3DTTFF_PROJECTED) == 0 || count < 3 || count > 4) {
        return tc;
    }
    float divisor = tc[count - 1];
    if (divisor == 0.0) {
        return tc;
    }
    if (divisor < 0.0) {
        clipped = true;
        return tc;
    }
    float4 projected = tc;
    projected.xy /= divisor;
    if (count == 4) {
        projected.z /= divisor;
    }
    clipped = any(projected.xy < 0.0) || any(projected.xy > 1.0);
    return projected;
}

static float4 computeTexCoord(constant Uniforms &uniforms, thread FragmentIn &in, uint tci, uint stage,
                              thread bool &clipped) {
    clipped = false;
    uint flags = uniforms.texTransformFlags[stage];
    if (uniforms.vertexProgramActive != 0) {
        return projectedTexCoord(vertexTexCoord(in, stage), flags, clipped);
    }

    uint mode = texGenMode(uniforms, tci);
    uint index = tci & 0x7;
    float4 tc = mode == TCI_PASSTHRU ? vertexTexCoord(in, index) : generatedTexCoord(in, mode);
    uint count = flags & 0xFF;
    if (count >= 2 && uniforms.useProjection == 1) {
        float4 source = mode == TCI_PASSTHRU ? texCoordAsTransformInput(tc, uniforms.texCoordSize[index]) : tc;
        tc = truncatedTexCoord(uniforms.texMatrix[stage] * source, count);
    }
    return projectedTexCoord(tc, flags, clipped);
}

static float4 unpackTexelFormat(float4 texel, uint format) {
    switch (format) {
        case TEXFORMAT_LUMINANCE:       return float4(texel.rrr, 1.0);
        case TEXFORMAT_LUMINANCE_ALPHA: return float4(texel.rrr, texel.g);
        case TEXFORMAT_DXT1:            return all(texel.rgb == 0.0) ? float4(0.0) : texel;
        case TEXFORMAT_NO_ALPHA:        return float4(texel.rgb, 1.0);
        case TEXFORMAT_BUMP_DUDV:       return float4(texel.rg, 1.0, 1.0);
        default:                        return texel;
    }
}
