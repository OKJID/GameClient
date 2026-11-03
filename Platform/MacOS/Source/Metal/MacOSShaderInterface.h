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
    float4x4 texMatrix[4];  // D3DTS_TEXTURE0..3 — UV transform matrices
    float2 screenSize;
    int useProjection;      // 0=passthrough, 1=3D, 2=2D screen space
    uint shaderSettings;    // legacy bitfield (texturing bit etc.)
    uint texTransformFlags[4]; // D3DTSS_TEXTURETRANSFORMFLAGS per stage (0=disabled, 2=COUNT2)
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
    uint _pad0;
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
#define D3DTOP_BLENDCURRENTALPHA  16
#define D3DTOP_MODULATEALPHA_ADDCOLOR   18
#define D3DTOP_MODULATECOLOR_ADDALPHA   19
#define D3DTOP_MODULATEINVALPHA_ADDCOLOR 20
#define D3DTOP_MODULATEINVCOLOR_ADDALPHA 21
#define D3DTOP_DOTPRODUCT3       24
#define D3DTOP_MULTIPLYADD       25
#define D3DTOP_LERP              26

// D3DTA source selectors (low 4 bits)
#define D3DTA_DIFFUSE  0
#define D3DTA_CURRENT  1
#define D3DTA_TEXTURE  2
#define D3DTA_TFACTOR  3
#define D3DTA_SPECULAR 4
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

struct TextureStageConfig {
    uint colorOp;     // D3DTOP enum
    uint colorArg1;   // D3DTA enum
    uint colorArg2;   // D3DTA enum
    uint alphaOp;     // D3DTOP enum
    uint alphaArg1;   // D3DTA enum
    uint alphaArg2;   // D3DTA enum
    uint colorArg0;   // D3DTA enum (3rd arg for MULTIPLYADD, LERP)
    uint _pad1;
};

struct FragmentUniforms {
    TextureStageConfig stages[4];
    float4 textureFactor;   // D3DRS_TEXTUREFACTOR (ARGB as float4)
    float4 fogColor;
    float  fogStart;
    float  fogEnd;
    float  fogDensity;
    uint   fogMode;
    uint   alphaTestEnable;
    uint   alphaFunc;        // D3DCMP enum
    float  alphaRef;         // normalized 0..1
    uint   hasTexture[4];
    uint   specularEnable;
    uint   texCoordIndex[4]; // D3DTSS_TEXCOORDINDEX: which UV set each stage uses
    uint   texFormatType[4]; // 0=Default, 1=Luminance(L8/P8), 2=Luminance+Alpha(A4L4/A8L8), 3=DXT1(BC1)
    uint   blendEnabled;     // D3DRS_ALPHABLENDENABLE — used by black discard guard
};

struct VertexOut {
    float4 position [[position]];
    float4 color;
    float4 specularColor;
    float2 texCoord;
    float2 texCoord2;
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
    float2 texCoord;
    float2 texCoord2;
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

static float3 computeTexCoord(constant Uniforms &uniforms, thread FragmentIn &in, uint tci, uint stage) {
    uint tciMode = (tci >> 16) & 0x3;
    uint uvIndex = tci & 0x3;
    float4 unprojected;

    if (tciMode == 1 && uniforms.useProjection == 1) {
        // D3DTSS_TCI_CAMERASPACEPOSITION (1)
        unprojected = float4(in.camPosX, in.camPosY, in.camPosZ, 1.0);
    } else if (tciMode == 2) {
        // D3DTSS_TCI_CAMERASPACENORMAL (2)
        unprojected = float4(in.camNormalX, in.camNormalY, in.camNormalZ, 1.0);
    } else if (tciMode == 3) {
        // D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR (3)
        float3 V = normalize(float3(in.camPosX, in.camPosY, in.camPosZ));
        float3 N = float3(in.camNormalX, in.camNormalY, in.camNormalZ);
        float3 R = V - 2.0 * dot(V, N) * N;
        unprojected = float4(R.x, R.y, R.z, 1.0);
    } else {
        // D3DTSS_TCI_PASSTHRU (0)
        float2 uv = (uvIndex == 1) ? in.texCoord2 : in.texCoord;
        unprojected = float4(uv.x, uv.y, 0.0, 1.0);
    }

    uint flags = uniforms.texTransformFlags[stage];
    if (flags != 0) {
        float4 tc = uniforms.texMatrix[stage] * unprojected;
        if ((flags & 256) != 0) { // D3DTTFF_PROJECTED
            uint count = flags & 255;
            if (count == 3 && tc.z != 0.0f) {
                if (tc.z <= 0.0f) return float3(0.0f, 0.0f, -1.0f);
                tc.x /= tc.z;
                tc.y /= tc.z;
                if (tc.x < 0.0f || tc.x > 1.0f || tc.y < 0.0f || tc.y > 1.0f) return float3(0.0f, 0.0f, -1.0f);
            } else if (count == 4 && tc.w != 0.0f) {
                if (tc.w <= 0.0f) return float3(0.0f, 0.0f, -1.0f);
                tc.x /= tc.w;
                tc.y /= tc.w;
                tc.z /= tc.w;
                if (tc.x < 0.0f || tc.x > 1.0f || tc.y < 0.0f || tc.y > 1.0f) return float3(0.0f, 0.0f, -1.0f);
            }
        }
        // Non-projected texgen (COUNT2): tc.z is the raw third matrix
        // component, not a projective clip coordinate. The fragment body
        // discards when any uv.z < 0 to emulate projected-texture clipping,
        // and only the .xy of the result is ever sampled — so leaking a raw
        // negative tc.z here made the projected terrain overlay (cloud/light
        // map) get discarded in bands wherever tc.z dipped below zero across
        // the near-flat lowland, producing shimmering horizontal stripes.
        // Force z=0 so only genuine projected clipping (the -1 sentinel
        // above) can trigger the discard.
        return float3(tc.xy, 0.0f);
    }
    return unprojected.xyz;
}
