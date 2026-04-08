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
    uint3 _pad;
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

// ─────────────────────────────────────────────────────
//  Stage 8: Lighting Uniforms (buffer 3)
//  Mirrors LightData / LightingUniforms in MetalDevice8.mm
// ─────────────────────────────────────────────────────

#define D3DLIGHT_POINT       1
#define D3DLIGHT_SPOT        2
#define D3DLIGHT_DIRECTIONAL 3

#define D3DMCS_MATERIAL 0
#define D3DMCS_COLOR1   1
#define D3DMCS_COLOR2   2

// D3DFOG modes
#define D3DFOG_NONE   0
#define D3DFOG_EXP    1
#define D3DFOG_EXP2   2
#define D3DFOG_LINEAR 3

struct LightData {
    float4 diffuse;
    float4 ambient;
    float4 specular;
    float3 position;
    float  range;
    float3 direction;
    float  falloff;
    float  attenuation0;
    float  attenuation1;
    float  attenuation2;
    float  theta;
    float  phi;
    uint   type;     // 1=point, 2=spot, 3=directional
    uint   enabled;
    float  _pad;
};

struct LightingUniforms {
    LightData lights[8];
    float4 materialDiffuse;
    float4 materialAmbient;
    float4 materialSpecular;
    float4 materialEmissive;
    float  materialPower;
    float4 globalAmbient;
    uint   lightingEnabled;
    uint   diffuseSource;
    uint   ambientSource;
    uint   specularSource;
    uint   emissiveSource;
    uint   hasNormals;
    uint   hasVertexColors;
    uint   colorVertexEnable;
    // Stage 9: Fog parameters (for vertex fog computation)
    float  fogStart;
    float  fogEnd;
    float  fogDensity;
    uint   fogMode; // 0=NONE, 1=EXP, 2=EXP2, 3=LINEAR
};

// ─────────────────────────────────────────────────────
//  Custom Vertex Shader Uniforms (buffer 4)
//  Passed from CPU when a DX8 custom vertex shader is active
// ─────────────────────────────────────────────────────
struct CustomVSUniforms {
    uint  shaderType;    // 0=none, 1=trees, 2=water wave
    uint  _pad[3];
    float4 c[34];         // VS constant registers c0..c33
};

// ─────────────────────────────────────────────────────
//  Custom Pixel Shader Uniforms (buffer 5)
//  Passed from CPU when a DX8 custom pixel shader is active
// ─────────────────────────────────────────────────────
struct CustomPSUniforms {
    uint  psType;        // PSType enum: 0=none, 1=terrain, 2=terrain+noise, etc.
    uint  _pad[3];
    float4 c[8];          // PS constant registers c0..c7
};

// ─────────────────────────────────────────────────────
//  Vertex I/O
// ─────────────────────────────────────────────────────

struct VertexIn {
    float4 position     [[attribute(0)]]; // float3 (auto-padded w=1) or float4 (XYZRHW)
    float4 color        [[attribute(1)]]; // diffuse vertex color (BGRA normalized)
    float2 texCoord     [[attribute(2)]]; // UV set 0
    float3 normal       [[attribute(3)]]; // vertex normal (for lighting)
    float4 specVtxColor [[attribute(4)]]; // specular vertex color (BGRA normalized)
    float2 texCoord2    [[attribute(5)]]; // UV set 1 (multi-texturing)
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

// ─────────────────────────────────────────────────────
//  Vertex Shader (with DX8 per-vertex lighting)
// ─────────────────────────────────────────────────────

vertex VertexOut vertex_main(VertexIn in [[stage_in]],
                            constant Uniforms &uniforms [[buffer(1)]],
                            constant LightingUniforms &lighting [[buffer(3)]],
                            constant CustomVSUniforms &customVS [[buffer(4)]]) {
    VertexOut out;
    
    float4 pos = float4(in.position.xyz, 1.0);
    
    auto applyClipPlanes = [&](float4 wPos, thread VertexOut& vOut) {
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
    };
    
    // ─── Custom Vertex Shader: Trees (shaderType == 1) ───
    // Implements Trees.vso: WVP transform, sway displacement, shroud UV generation
    if (customVS.shaderType == 1) {
        // c4-c7: Transposed World-View-Projection matrix (row-major in constants)
        float4x4 wvpT = float4x4(customVS.c[4], customVS.c[5], customVS.c[6], customVS.c[7]);
        
        // Sway displacement:
        // The tree vertex shader encodes swayType in the diffuse alpha channel.
        // The vertex normal.z (height above ground) weights the sway amount.
        // c8 = no-sway sentinel (always 0,0,0,0)
        // c9..c9+MAX_SWAY_TYPES = sway vectors per type
        // SwayType is encoded in vertex data during tree buffer construction.
        // In the original shader: swayType stored in vertex diffuse alpha as 1-based index.
        //
        // Extract sway type from diffuse alpha (packed as 0..MAX_SWAY_TYPES normalized)
        // Extract sway type from normal.x (repurposed field).
        // In W3DTreeBuffer, swayType is stored in curVb->nx = swayType (1..MAX_SWAY_TYPES)
        // as an integer value packed into the normal.x float component.
        uint swayType = (uint)(in.normal.x + 0.5);
        if (swayType < 1) swayType = 1;
        if (swayType > 8) swayType = 8;
        
        // Sway vector for this tree's sway type is in c[8 + swayType]
        float3 swayVec = customVS.c[8 + swayType].xyz;
        
        // Weight sway by vertex height above ground:
        // In W3DTreeBuffer, normal is repurposed: nx=swayType, ny=darkening, nz=ground Z.
        // Trees.vso computes sway weight as (v0.z - v1.z) = vertex Z - ground Z
        // = height of this vertex above the tree's base. Top sways more, base doesn't.
        float swayWeight = pos.z - in.normal.z;
        
        // Apply sway displacement to position
        float4 swayedPos = pos;
        swayedPos.xyz += swayVec * swayWeight;
        
        // Transform: WVP matrix is transposed, so multiply as pos * wvpT
        out.position = swayedPos * wvpT;
        
        // Texture coordinates: pass through UV0
        out.texCoord = in.texCoord;
        
        // Shroud UV generation from c32 (offset) and c33 (scale)
        // shroudUV.x = (worldPos.x + c32.x) * c33.x
        // shroudUV.y = (worldPos.y + c32.y) * c33.y
        float2 shroudOffset = customVS.c[32].xy;
        float2 shroudScale = customVS.c[33].xy;
        out.texCoord2 = float2((swayedPos.x + shroudOffset.x) * shroudScale.x,
                               (swayedPos.y + shroudOffset.y) * shroudScale.y);
        
        // Diffuse color: pass through but restore alpha to 1.0
        // (alpha channel was used to encode swayType, not actual alpha)
        out.color = float4(in.color.rgb, 1.0);
        out.specularColor = float4(0.0);
        
        // Camera-space position for TCI
        float4 csPos = uniforms.view * uniforms.world * swayedPos;
        out.camPosX = csPos.x;
        out.camPosY = csPos.y;
        out.camPosZ = csPos.z;
        out.camNormalX = 0.0;
        out.camNormalY = 0.0;
        out.camNormalZ = 1.0;
        
        // Fog
        float dist = length(csPos.xyz);
        if (lighting.fogMode == D3DFOG_LINEAR) {
            float fogRange = lighting.fogEnd - lighting.fogStart;
            out.fogFactor = (fogRange > 0.0001) ? clamp((lighting.fogEnd - dist) / fogRange, 0.0, 1.0) : 1.0;
        } else if (lighting.fogMode == D3DFOG_EXP) {
            out.fogFactor = clamp(exp(-lighting.fogDensity * dist), 0.0, 1.0);
        } else if (lighting.fogMode == D3DFOG_EXP2) {
            float e = lighting.fogDensity * dist;
            out.fogFactor = clamp(exp(-(e * e)), 0.0, 1.0);
        } else {
            out.fogFactor = 1.0;
        }
        
        applyClipPlanes(swayedPos, out);
        return out;
    }
    
    // ─── Custom Vertex Shader: Water Wave (shaderType == 2) ───
    // Implements wave.vso: WVP transform, texture projection for reflection
    if (customVS.shaderType == 2) {
        // c2-c5: Transposed World-View-Projection matrix
        float4x4 wvpT = float4x4(customVS.c[2], customVS.c[3], customVS.c[4], customVS.c[5]);
        
        // Transform position
        out.position = pos * wvpT;
        
        // UV0: pass through
        out.texCoord = in.texCoord;
        
        // UV1: texture projection for reflection
        // c6-c9: Transposed texture projection matrix
        float4x4 texProjT = float4x4(customVS.c[6], customVS.c[7], customVS.c[8], customVS.c[9]);
        float4 projUV = pos * texProjT;
        out.texCoord2 = projUV.xy;
        
        // Diffuse color
        out.color = in.color;
        out.specularColor = float4(0.0);
        
        // Camera space
        out.camPosX = 0.0;
        out.camPosY = 0.0;
        out.camPosZ = 0.0;
        
        // Fog
        float4 viewPos = uniforms.view * uniforms.world * pos;
        float dist = length(viewPos.xyz);
        if (lighting.fogMode == D3DFOG_LINEAR) {
            float fogRange = lighting.fogEnd - lighting.fogStart;
            out.fogFactor = (fogRange > 0.0001) ? clamp((lighting.fogEnd - dist) / fogRange, 0.0, 1.0) : 1.0;
        } else {
            out.fogFactor = 1.0;
        }
        
        applyClipPlanes(pos, out);
        return out;
    }
    
    // ─── Standard vertex shader (no custom VS) ───
    if (uniforms.useProjection == 1) {
        // 3D Mode
        out.position = uniforms.projection * uniforms.view * uniforms.world * pos;
    } else if (uniforms.useProjection == 2) {
        // 2D Mode (Screen Space / XYZRHW)
        // XYZRHW: x,y are screen coords, z is depth [0..1], w is 1/z (rhw)
        float2 screenPos = (in.position.xy / uniforms.screenSize) * 2.0 - 1.0;
        float z = in.position.z; // depth value from vertex
        float rhw = in.position.w; // reciprocal homogeneous w
        out.position = float4(screenPos.x, -screenPos.y, z, 1.0);
    } else {
        out.position = pos;
    }
    
    out.texCoord = in.texCoord;
    out.texCoord2 = in.texCoord2;
    
    // Compute camera-space position for D3DTSS_TCI_CAMERASPACEPOSITION
    float4 worldPos = uniforms.world * pos;
    float4 camPos = uniforms.view * worldPos;
    out.camPosX = camPos.x;
    out.camPosY = camPos.y;
    out.camPosZ = camPos.z;
    out.camNormalX = 0.0; // default for 2D UI elements
    out.camNormalY = 0.0;
    out.camNormalZ = 1.0;
    
    out.specularColor = float4(0.0, 0.0, 0.0, 0.0);
    
    // For 2D/screen-space vertices (XYZRHW), skip fog entirely.
    // The world/view transforms are meaningless for screen-space coords,
    // and applying fog to UI elements would make them invisible.
    if (uniforms.useProjection == 2) {
        out.fogFactor = 1.0; // fully visible, no fog
    } else {
        // 3D fog distance — computed properly using DX8 fog formulas
        float4 viewPos = uniforms.view * uniforms.world * pos;
        float dist = length(viewPos.xyz); // distance from camera in view space
        
        // Compute fog factor based on fog mode from LightingUniforms
        // fogFactor: 1.0 = no fog (fully visible), 0.0 = fully fogged
        if (lighting.fogMode == D3DFOG_LINEAR) {
            float fogRange = lighting.fogEnd - lighting.fogStart;
            if (fogRange > 0.0001) {
                out.fogFactor = clamp((lighting.fogEnd - dist) / fogRange, 0.0, 1.0);
            } else {
                out.fogFactor = (dist < lighting.fogEnd) ? 1.0 : 0.0;
            }
        } else if (lighting.fogMode == D3DFOG_EXP) {
            out.fogFactor = exp(-lighting.fogDensity * dist);
            out.fogFactor = clamp(out.fogFactor, 0.0, 1.0);
        } else if (lighting.fogMode == D3DFOG_EXP2) {
            float exponent = lighting.fogDensity * dist;
            out.fogFactor = exp(-(exponent * exponent));
            out.fogFactor = clamp(out.fogFactor, 0.0, 1.0);
        } else {
            // D3DFOG_NONE — no fog
            out.fogFactor = 1.0; // fully visible
        }
    }
    
    // ─── Per-vertex lighting ───
    if (lighting.lightingEnabled == 0 || lighting.hasNormals == 0) {
        // Lighting disabled or no normals: pass through vertex color
        out.color = in.color;
        return out;
    }
    
    // Transform vertex position and normal to view space
    float4x4 worldView = uniforms.view * uniforms.world;
    float3 posVS = (worldView * pos).xyz;
    
    // Transform normal to view space using upper-left 3x3 of worldView.
    // To handle non-uniform scale and mirrored scaling accurately, we evaluate
    // the cofactor matrix (which represents InverseTranspose scaled by determinant).
    // We multiply by the sign of the determinant to prevent inward-facing normals on mirrored meshes!
    float3x3 M_worldView = float3x3(worldView[0].xyz, worldView[1].xyz, worldView[2].xyz);
    
    float3x3 M_cof;
    M_cof[0][0] = M_worldView[1][1]*M_worldView[2][2] - M_worldView[1][2]*M_worldView[2][1];
    M_cof[0][1] = M_worldView[1][2]*M_worldView[2][0] - M_worldView[1][0]*M_worldView[2][2];
    M_cof[0][2] = M_worldView[1][0]*M_worldView[2][1] - M_worldView[1][1]*M_worldView[2][0];
    
    M_cof[1][0] = M_worldView[2][1]*M_worldView[0][2] - M_worldView[2][2]*M_worldView[0][1];
    M_cof[1][1] = M_worldView[2][2]*M_worldView[0][0] - M_worldView[2][0]*M_worldView[0][2];
    M_cof[1][2] = M_worldView[2][0]*M_worldView[0][1] - M_worldView[2][1]*M_worldView[0][0];
    
    M_cof[2][0] = M_worldView[0][1]*M_worldView[1][2] - M_worldView[0][2]*M_worldView[1][1];
    M_cof[2][1] = M_worldView[0][2]*M_worldView[1][0] - M_worldView[0][0]*M_worldView[1][2];
    M_cof[2][2] = M_worldView[0][0]*M_worldView[1][1] - M_worldView[0][1]*M_worldView[1][0];

    float det = M_worldView[0][0]*M_cof[0][0] + M_worldView[0][1]*M_cof[0][1] + M_worldView[0][2]*M_cof[0][2];
    float3 N_raw = M_cof * in.normal;
    if (det < 0.0) {
        N_raw = -N_raw;
    }
    float3 N = any(N_raw != 0.0f) ? normalize(N_raw) : float3(0.0f, 1.0f, 0.0f);
    out.camNormalX = N.x;
    out.camNormalY = N.y;
    out.camNormalZ = N.z;
    
    // Resolve material source colors per DX8 spec
    // If a mesh does not have D3DFVF_DIFFUSE, DX8 still expands the vertex providing a default White color for COLOR1.
    // D3DRS_COLORVERTEX dictates whether vertex color is evaluated at all.
    float4 col1 = lighting.hasVertexColors ? in.color : float4(1.0);
    float4 col2 = lighting.hasVertexColors ? in.specVtxColor : float4(0.0);
    
    bool colorVertex = (lighting.colorVertexEnable != 0);

    float4 matDiffuse  = (colorVertex && lighting.diffuseSource  == D3DMCS_COLOR1) ? col1 :
                         (colorVertex && lighting.diffuseSource  == D3DMCS_COLOR2) ? col2 :
                         lighting.materialDiffuse;
    
    float4 matAmbient  = (colorVertex && lighting.ambientSource  == D3DMCS_COLOR1) ? col1 :
                         (colorVertex && lighting.ambientSource  == D3DMCS_COLOR2) ? col2 :
                         lighting.materialAmbient;
    
    float4 matSpecular = (colorVertex && lighting.specularSource == D3DMCS_COLOR1) ? col1 :
                         (colorVertex && lighting.specularSource == D3DMCS_COLOR2) ? col2 :
                         lighting.materialSpecular;
    
    float4 matEmissive = (colorVertex && lighting.emissiveSource == D3DMCS_COLOR1) ? col1 :
                         (colorVertex && lighting.emissiveSource == D3DMCS_COLOR2) ? col2 :
                         lighting.materialEmissive;
    
    // Accumulate lighting components
    float4 totalDiffuse  = float4(0.0);
    float4 totalSpecular = float4(0.0);
    float4 totalAmbient  = lighting.globalAmbient; // start with global ambient Ga
    
    for (int i = 0; i < 8; i++) {
        if (lighting.lights[i].enabled == 0) continue;
        
        constant LightData &light = lighting.lights[i];
        
        float3 Ldir;    // direction FROM vertex TO light (or light direction for directional)
        float atten = 1.0;
        float spot  = 1.0;
        
        if (light.type == D3DLIGHT_DIRECTIONAL) {
            // Directional: Ldir comes from DX8 as direction the light shines
            // In view space: transform then negate
            Ldir = -normalize(float3x3(uniforms.view[0].xyz, uniforms.view[1].xyz, uniforms.view[2].xyz) * light.direction);
            atten = 1.0;
        } else {
            // Point or Spot light
            float3 lightPosVS = (uniforms.view * float4(light.position, 1.0)).xyz;
            float3 toLight = lightPosVS - posVS;
            float d = length(toLight);
            
            // Range check
            if (d > light.range && light.range > 0.0) continue;
            
            Ldir = toLight / max(d, 0.0001);
            
            // Attenuation: 1 / (a0 + a1*d + a2*d²)
            float denominator = light.attenuation0 + light.attenuation1 * d + light.attenuation2 * d * d;
            atten = 1.0 / max(denominator, 0.0001);
            atten = min(atten, 1.0);
            
            if (light.type == D3DLIGHT_SPOT) {
                // Spotlight cone
                float3 spotDirVS = normalize(float3x3(uniforms.view[0].xyz, uniforms.view[1].xyz, uniforms.view[2].xyz) * light.direction);
                float rho = dot(-Ldir, spotDirVS);
                float cosHalfTheta = cos(light.theta * 0.5);
                float cosHalfPhi   = cos(light.phi * 0.5);
                
                if (rho <= cosHalfPhi) {
                    spot = 0.0;
                } else if (rho >= cosHalfTheta) {
                    spot = 1.0;
                } else {
                    float num = rho - cosHalfPhi;
                    float den = cosHalfTheta - cosHalfPhi;
                    spot = pow(max(num / max(den, 0.0001), 0.0), max(light.falloff, 0.0001));
                }
            }
        }
        
        // Ambient contribution from this light
        totalAmbient += light.ambient;
        
        // Diffuse: N·L (clamped)
        float NdotL = max(dot(N, Ldir), 0.0);
        totalDiffuse += light.diffuse * NdotL * atten * spot;
        
        // Specular: (N·H)^power
        if (NdotL > 0.0 && lighting.materialPower > 0.0) {
            // Halfway vector: H = normalize(Ldir + V)
            // Use local viewer approximation from posVS
            float3 V = normalize(-posVS);
            float3 H = normalize(Ldir + V);
            float NdotH = max(dot(N, H), 0.0);
            float specPow = pow(NdotH, lighting.materialPower);
            totalSpecular += light.specular * specPow * atten * spot;
        }
    }
    
    // Final color = emissive + ambient * materialAmbient + diffuse * materialDiffuse
    float4 finalColor;
    finalColor.rgb = matEmissive.rgb
                   + matAmbient.rgb * totalAmbient.rgb
                   + matDiffuse.rgb * totalDiffuse.rgb;
    finalColor.a = matDiffuse.a; // DX8: output alpha = material diffuse alpha
    finalColor = clamp(finalColor, 0.0, 1.0);
    
    out.color = finalColor;
    out.specularColor = float4(matSpecular.rgb * totalSpecular.rgb, 0.0);
    out.specularColor = clamp(out.specularColor, 0.0, 1.0);
    
    applyClipPlanes(uniforms.world * pos, out);
    return out;
}

// ─────────────────────────────────────────────────────
//  Stage 7: TSS Argument Resolver
//  Selects the color/alpha source based on D3DTA value
// ─────────────────────────────────────────────────────
float4 resolveArg(uint argId,
                  float4 texColor0, float4 texColor1, float4 texColor2, float4 texColor3,
                  float4 diffuse,
                  float4 specular,
                  float4 current,
                  float4 tFactor,
                  uint   stageIndex) {
    uint source = argId & 0xF;  // low 4 bits = source selector
    float4 val;
    
    switch (source) {
        case D3DTA_DIFFUSE:  val = diffuse;  break;
        case D3DTA_CURRENT:  val = current;  break;
        case D3DTA_TEXTURE:  
            val = (stageIndex == 0) ? texColor0 :
                  (stageIndex == 1) ? texColor1 :
                  (stageIndex == 2) ? texColor2 : texColor3;
            break;
        case D3DTA_TFACTOR:  val = tFactor;  break;
        case D3DTA_SPECULAR: val = specular; break;
        default:             val = current;  break;
    }
    
    // Apply modifiers
    if (argId & D3DTA_COMPLEMENT)     val = 1.0 - val;
    if (argId & D3DTA_ALPHAREPLICATE) val = float4(val.a, val.a, val.a, val.a);
    
    return val;
}

// ─────────────────────────────────────────────────────
//  Stage 7: TSS Operation Evaluator
//  Computes result for both color (.rgb) and alpha (.a)
// ─────────────────────────────────────────────────────
float4 evaluateOp(uint op, float4 arg1, float4 arg2, float4 arg0) {
    switch (op) {
        case D3DTOP_DISABLE:
            return arg1;
        case D3DTOP_SELECTARG1:
            return arg1;
        case D3DTOP_SELECTARG2:
            return arg2;
        case D3DTOP_MODULATE:
            return arg1 * arg2;
        case D3DTOP_MODULATE2X:
            return clamp(arg1 * arg2 * 2.0, 0.0, 1.0);
        case D3DTOP_MODULATE4X:
            return clamp(arg1 * arg2 * 4.0, 0.0, 1.0);
        case D3DTOP_ADD:
            return clamp(arg1 + arg2, 0.0, 1.0);
        case D3DTOP_ADDSIGNED:
            return clamp(arg1 + arg2 - 0.5, 0.0, 1.0);
        case D3DTOP_ADDSIGNED2X:
            return clamp((arg1 + arg2 - 0.5) * 2.0, 0.0, 1.0);
        case D3DTOP_SUBTRACT:
            return clamp(arg1 - arg2, 0.0, 1.0);
        case D3DTOP_ADDSMOOTH:
            return clamp(arg1 + arg2 - arg1 * arg2, 0.0, 1.0);
        case D3DTOP_BLENDDIFFUSEALPHA: {
            return arg1; // Handled in evaluateBlendOp
        }
        case D3DTOP_BLENDTEXTUREALPHA: {
            return arg1; // Handled in evaluateBlendOp
        }
        case D3DTOP_BLENDFACTORALPHA: {
            return arg1; // Handled in evaluateBlendOp
        }
        case D3DTOP_BLENDCURRENTALPHA: {
            return arg1; // Handled in evaluateBlendOp
        }
        case D3DTOP_MODULATEALPHA_ADDCOLOR:
            return float4(arg1.rgb + arg1.a * arg2.rgb, arg1.a);
        case D3DTOP_MODULATECOLOR_ADDALPHA:
            return float4(arg1.rgb * arg2.rgb + arg1.a, arg1.a);
        case D3DTOP_MODULATEINVALPHA_ADDCOLOR:
            return float4((1.0 - arg1.a) * arg2.rgb + arg1.rgb, arg1.a);
        case D3DTOP_MODULATEINVCOLOR_ADDALPHA:
            return float4((1.0 - arg1.rgb) * arg2.rgb + float3(arg1.a), arg1.a);
        case D3DTOP_DOTPRODUCT3: {
            float d = dot(arg1.rgb * 2.0 - 1.0, arg2.rgb * 2.0 - 1.0);
            return float4(d, d, d, d);
        }
        case D3DTOP_MULTIPLYADD:
            return clamp(arg0 + arg1 * arg2, 0.0, 1.0);
        case D3DTOP_LERP:
            return clamp(arg1 * arg2 + (1.0 - arg1) * arg0, 0.0, 1.0);
        default:
            return arg1 * arg2;
    }
}

// ─────────────────────────────────────────────────────
//  Stage 7: Blend operation helper
//  For BLENDDIFFUSEALPHA, BLENDTEXTUREALPHA, etc.
// ─────────────────────────────────────────────────────
float4 evaluateBlendOp(uint op, float4 arg1, float4 arg2, float4 arg0,
                       float4 diffuse, float4 stageTexColor, float4 tFactor,
                       float4 current) {
    float alpha;
    switch (op) {
        case D3DTOP_BLENDDIFFUSEALPHA:
            alpha = diffuse.a;
            return float4(mix(arg2.rgb, arg1.rgb, alpha),
                         mix(arg2.a, arg1.a, alpha));
        case D3DTOP_BLENDTEXTUREALPHA:
            alpha = stageTexColor.a;
            return float4(mix(arg2.rgb, arg1.rgb, alpha),
                         mix(arg2.a, arg1.a, alpha));
        case D3DTOP_BLENDFACTORALPHA:
            alpha = tFactor.a;
            return float4(mix(arg2.rgb, arg1.rgb, alpha),
                         mix(arg2.a, arg1.a, alpha));
        case D3DTOP_BLENDCURRENTALPHA:
            alpha = current.a;
            return float4(mix(arg2.rgb, arg1.rgb, alpha),
                         mix(arg2.a, arg1.a, alpha));
        default:
            return evaluateOp(op, arg1, arg2, arg0);
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

// ─────────────────────────────────────────────────────
//  Stage 7: Fragment Shader with full TSS support
// ─────────────────────────────────────────────────────
fragment float4 fragment_main(FragmentIn in [[stage_in]],
                             constant Uniforms &uniforms [[buffer(1)]],
                             constant FragmentUniforms &fragUniforms [[buffer(2)]],
                             constant CustomPSUniforms &psUniforms [[buffer(5)]],
                             texture2d<float> tex0 [[texture(0)]],
                             texture2d<float> tex1 [[texture(1)]],
                             texture2d<float> tex2 [[texture(2)]],
                             texture2d<float> tex3 [[texture(3)]],
                             texturecube<float> texCube0 [[texture(4)]],
                             texturecube<float> texCube1 [[texture(5)]],
                             texturecube<float> texCube2 [[texture(6)]],
                             texturecube<float> texCube3 [[texture(7)]],
                             sampler sampler0 [[sampler(0)]],
                             sampler sampler1 [[sampler(1)]],
                             sampler sampler2 [[sampler(2)]],
                             sampler sampler3 [[sampler(3)]]) {
    auto computeTexCoord = [&](uint tci, uint stage) -> float3 {
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
            return tc.xyz;
        }
        return unprojected.xyz;
    };

    // ════════════════════════════════════════════════════
    //  Custom Pixel Shader path — bypasses TSS completely
    //  When a DX8 pixel shader is active, the engine does NOT set TSS states.
    //  Instead, we execute the equivalent PS logic here.
    // ════════════════════════════════════════════════════
    if (psUniforms.psType != 0) {
        // Select UV coordinates (PS uses tex coord index from TSS states)

        float3 psUV0 = computeTexCoord(fragUniforms.texCoordIndex[0], 0);
        float3 psUV1 = computeTexCoord(fragUniforms.texCoordIndex[1], 1);
        float3 psUV2 = computeTexCoord(fragUniforms.texCoordIndex[2], 2);
        float3 psUV3 = computeTexCoord(fragUniforms.texCoordIndex[3], 3);

        float4 t0 = (fragUniforms.hasTexture[0] == 2) ? texCube0.sample(sampler0, psUV0) : 
                    (fragUniforms.hasTexture[0] != 0) ? tex0.sample(sampler0, psUV0.xy) : float4(1.0);
        float4 t1 = (fragUniforms.hasTexture[1] == 2) ? texCube1.sample(sampler1, psUV1) : 
                    (fragUniforms.hasTexture[1] != 0) ? tex1.sample(sampler1, psUV1.xy) : float4(1.0);
        float4 t2 = (fragUniforms.hasTexture[2] == 2) ? texCube2.sample(sampler2, psUV2) : 
                    (fragUniforms.hasTexture[2] != 0) ? tex2.sample(sampler2, psUV2.xy) : float4(1.0);
        float4 t3 = (fragUniforms.hasTexture[3] == 2) ? texCube3.sample(sampler3, psUV3) : 
                    (fragUniforms.hasTexture[3] != 0) ? tex3.sample(sampler3, psUV3.xy) : float4(1.0);

        float4 diffuse = in.color;
        float4 result = float4(0.0);

        uint psType = psUniforms.psType;

        if (psType == 1) {
            // PS_TERRAIN: lrp r0, v0.a, t1, t0 => r0 = t1*a + t0*(1-a)
            // DX8 lrp dest, factor, src1, src2 = src1*factor + src2*(1-factor)
            float a = diffuse.a;
            result.rgb = mix(t0.rgb, t1.rgb, a) * diffuse.rgb;
            result.a = 1.0;
        }
        else if (psType == 2) {
            // PS_TERRAIN_NOISE1: terrain + cloud texture on stage 2
            // lrp r0, v0.a, t1, t0 => t1*a + t0*(1-a)
            float a = diffuse.a;
            float4 terrainBlend;
            terrainBlend.rgb = mix(t0.rgb, t1.rgb, a) * diffuse.rgb;
            terrainBlend.a = 1.0;
            // Multiply by cloud/noise texture
            result.rgb = terrainBlend.rgb * t2.rgb;
            result.a = 1.0;
        }
        else if (psType == 3) {
            // PS_TERRAIN_NOISE2: terrain + cloud + noise
            // lrp r0, v0.a, t1, t0 => t1*a + t0*(1-a)
            float a = diffuse.a;
            float4 terrainBlend;
            terrainBlend.rgb = mix(t0.rgb, t1.rgb, a) * diffuse.rgb;
            terrainBlend.a = 1.0;
            // Multiply by cloud * noise
            result.rgb = terrainBlend.rgb * t2.rgb * t3.rgb;
            result.a = 1.0;
        }
        else if (psType == 4) {
            // PS_ROAD_NOISE2: road + cloud + noise
            result.rgb = t0.rgb * t1.rgb * t2.rgb;
            result.a = t0.a;
        }
        else if (psType == 5) {
            // PS_MONOCHROME: luminance = dot(color, weights) from c0
            float4 weights = psUniforms.c[0]; // typically (0.3, 0.59, 0.11, 1.0)
            float lum = dot(t0.rgb, weights.rgb);
            // Apply fade from c1 (mulColor) and c2 (fade)
            float4 mulColor = psUniforms.c[1];
            float4 fade = psUniforms.c[2];
            result.rgb = float3(lum) * mulColor.rgb * fade.rgb;
            result.a = t0.a;
        }
        else if (psType == 6) {
            // PS_WAVE: bump-mapped water with PS constant c0 (reflection factor)
            float4 reflFactor = psUniforms.c[0];
            result.rgb = t1.rgb * reflFactor.rgb;
            result.a = 1.0;
        }
        else if (psType == 7) {
            // PS_FLAT_TERRAIN (fterrain.pso): mul r0, t1, t0; mul r0, r0, v0
            result.rgb = t1.rgb * t0.rgb * diffuse.rgb;
            result.a = 1.0;
        }
        else if (psType == 8) {
            // PS_FLAT_TERRAIN0 (fterrain0.pso): mov r0, t1; mul r0, r0, v0
            result.rgb = t1.rgb * diffuse.rgb;
            result.a = 1.0;
        }
        else if (psType == 9) {
            // PS_FLAT_TERRAIN_NOISE1 (fterrainnoise.pso): mul chain
            result.rgb = t1.rgb * t0.rgb * diffuse.rgb * t2.rgb;
            result.a = 1.0;
        }
        else if (psType == 10) {
            // PS_FLAT_TERRAIN_NOISE2 (fterrainnoise2.pso): mul chain
            result.rgb = t1.rgb * t0.rgb * diffuse.rgb * t2.rgb * t3.rgb;
            result.a = 1.0;
        }
        else if (psType == 11) {
            // PS_WATER_TRAPEZOID: standing water with sparkles and shroud
            // Original PS1.1:
            //   mul r0, v0, t0         ; blend vertex color and alpha into base texture
            //   mad r0.rgb, t1, t2, r0 ; blend sparkles (t1 * t2) and add to r0
            //   mul r0.rgb, r0, t3     ; blend in black shroud
            result = diffuse * t0;                    // mul r0, v0, t0
            result.rgb = t1.rgb * t2.rgb + result.rgb;  // mad r0.rgb, t1, t2, r0
            result.rgb = result.rgb * t3.rgb;           // mul r0.rgb, r0, t3
        }
        else if (psType == 12) {
            // PS_WATER_BUMP: bump-mapped water with environment mapped reflection
            // Original PS1.1:
            //   tex t0
            //   tex t1
            //   texbem t2, t1        ; use t1 as env map adjustment on t2
            //   mul r0, v0, t0       ; blend vertex color into base texture
            //   mul r1.rgb, t2, c0   ; reduce environment reflection by constant
            //   add r0.rgb, r0, r1   ; add reflection
            //
            // texbem is not feasible in Metal without bump map textures,
            // so we approximate: use t2 directly as reflection with c0 factor
            float4 reflFactor = psUniforms.c[0];
            result = diffuse * t0;                         // mul r0, v0, t0
            result.rgb = result.rgb + t2.rgb * reflFactor.rgb; // add r0.rgb, r0, r1
        }
        else if (psType == 13) {
            // PS_WATER_RIVER: river water with sparkles and shroud
            // Original PS1.1:
            //   mul r0, v0, t0       ; blend vertex color into t0
            //   mul r1, t1, t2       ; sparkle * noise
            //   add r0.rgb, r0, t3   ; add shroud lighting
            //   +mul r0.a, r0, t3    ; multiply alpha by shroud alpha
            //   add r0.rgb, r0, r1   ; add sparkles
            result = diffuse * t0;                        // mul r0, v0, t0
            float3 sparkle = t1.rgb * t2.rgb;             // mul r1, t1, t2
            result.rgb = result.rgb + t3.rgb;             // add r0.rgb, r0, t3
            result.a = result.a * t3.a;                   // +mul r0.a, r0, t3
            result.rgb = result.rgb + sparkle;            // add r0.rgb, r0, r1
        }
        else {
            // Unknown PS: fallthrough to basic texturing
            result = t0 * diffuse;
        }

        // Alpha test
        if (fragUniforms.alphaTestEnable != 0) {
            if (!alphaTestPass(fragUniforms.alphaFunc, result.a, fragUniforms.alphaRef)) {
                discard_fragment();
            }
        }
        // Fog
        if (fragUniforms.fogMode != 0) {
            result.rgb = mix(fragUniforms.fogColor.rgb, result.rgb, in.fogFactor);
        }
        return result;
    }

    // ════════════════════════════════════════════════════
    //  TSS (Texture Stage State) path — fallback when no PS active
    // ════════════════════════════════════════════════════
    
    float3 uv0 = computeTexCoord(fragUniforms.texCoordIndex[0], 0);

    float3 uv1 = computeTexCoord(fragUniforms.texCoordIndex[1], 1);
    float3 uv2 = computeTexCoord(fragUniforms.texCoordIndex[2], 2);
    float3 uv3 = computeTexCoord(fragUniforms.texCoordIndex[3], 3);
    
    // Sample textures using their respective UV coordinates.
    // If projective coordinates were truncated (tc.z < 0), DX8 hardware would have clipped this geometry.
    // Emulate geometry clipping by completely discarding the fragment, preventing it from applying base diffuse logic.
    if (uv0.z < 0.0f || uv1.z < 0.0f || uv2.z < 0.0f || uv3.z < 0.0f) {
        discard_fragment();
    }
    
    float4 texColor0 = (fragUniforms.hasTexture[0] == 2) ? texCube0.sample(sampler0, uv0) :
                       (fragUniforms.hasTexture[0] != 0) ? tex0.sample(sampler0, uv0.xy) : float4(1.0f);
    float4 texColor1 = (fragUniforms.hasTexture[1] == 2) ? texCube1.sample(sampler1, uv1) :
                       (fragUniforms.hasTexture[1] != 0) ? tex1.sample(sampler1, uv1.xy) : float4(1.0f);
    float4 texColor2 = (fragUniforms.hasTexture[2] == 2) ? texCube2.sample(sampler2, uv2) :
                       (fragUniforms.hasTexture[2] != 0) ? tex2.sample(sampler2, uv2.xy) : float4(1.0f);
    float4 texColor3 = (fragUniforms.hasTexture[3] == 2) ? texCube3.sample(sampler3, uv3) :
                       (fragUniforms.hasTexture[3] != 0) ? tex3.sample(sampler3, uv3.xy) : float4(1.0f);
    
    // Apply texture format data unpacking
    if (fragUniforms.texFormatType[0] == 1) texColor0 = float4(texColor0.r, texColor0.r, texColor0.r, 1.0f);
    else if (fragUniforms.texFormatType[0] == 2) texColor0 = float4(texColor0.r, texColor0.r, texColor0.r, texColor0.g);
    
    if (fragUniforms.texFormatType[1] == 1) texColor1 = float4(texColor1.r, texColor1.r, texColor1.r, 1.0f);
    else if (fragUniforms.texFormatType[1] == 2) texColor1 = float4(texColor1.r, texColor1.r, texColor1.r, texColor1.g);
    
    if (fragUniforms.texFormatType[2] == 1) texColor2 = float4(texColor2.r, texColor2.r, texColor2.r, 1.0f);
    else if (fragUniforms.texFormatType[2] == 2) texColor2 = float4(texColor2.r, texColor2.r, texColor2.r, texColor2.g);
    
    if (fragUniforms.texFormatType[3] == 1) texColor3 = float4(texColor3.r, texColor3.r, texColor3.r, 1.0f);
    else if (fragUniforms.texFormatType[3] == 2) texColor3 = float4(texColor3.r, texColor3.r, texColor3.r, texColor3.g);

    // BC1 (DXT1) format decompression emulation. Empty blocks output exactly 0.0f into RGB.
    // Using MSL 'all' spec operator explicitly avoids unsafe floating point approximation thresholds.
    if (fragUniforms.texFormatType[0] == 3 && all(texColor0.rgb == 0.0f)) texColor0.a = 0.0f;
    if (fragUniforms.texFormatType[1] == 3 && all(texColor1.rgb == 0.0f)) texColor1.a = 0.0f;
    if (fragUniforms.texFormatType[2] == 3 && all(texColor2.rgb == 0.0f)) texColor2.a = 0.0f;
    if (fragUniforms.texFormatType[3] == 3 && all(texColor3.rgb == 0.0f)) texColor3.a = 0.0f;

    float4 diffuse = in.color;
    

    // Full TSS processing for 2D and textured 3D draws
    float4 specular = in.specularColor;
    float4 tFactor = fragUniforms.textureFactor;
    float4 current = diffuse;

    for (int i = 0; i < 4; i++) {
        uint colorOp = fragUniforms.stages[i].colorOp;
        if (colorOp == 0 || colorOp == D3DTOP_DISABLE) break;

        // Determine current stage texture for BLENDTEXTUREALPHA
        float4 stageTexColor = (i == 0) ? texColor0 : (i == 1) ? texColor1 : (i == 2) ? texColor2 : texColor3;

        float4 cArg1 = resolveArg(fragUniforms.stages[i].colorArg1, texColor0, texColor1, texColor2, texColor3, diffuse, specular, current, tFactor, i);
        float4 cArg2 = resolveArg(fragUniforms.stages[i].colorArg2, texColor0, texColor1, texColor2, texColor3, diffuse, specular, current, tFactor, i);
        float4 cArg0 = resolveArg(fragUniforms.stages[i].colorArg0, texColor0, texColor1, texColor2, texColor3, diffuse, specular, current, tFactor, i);
        float4 colorResult = evaluateBlendOp(colorOp, cArg1, cArg2, cArg0, diffuse, stageTexColor, tFactor, current);
        
        uint alphaOp = fragUniforms.stages[i].alphaOp;
        float alphaVal;
        if (alphaOp == 0 || alphaOp == D3DTOP_DISABLE) {
            // D3DTOP_DISABLE for alpha means "don't modify alpha from previous stage"
            alphaVal = current.a;
        } else {
            float4 aArg1 = resolveArg(fragUniforms.stages[i].alphaArg1, texColor0, texColor1, texColor2, texColor3, diffuse, specular, current, tFactor, i);
            float4 aArg2 = resolveArg(fragUniforms.stages[i].alphaArg2, texColor0, texColor1, texColor2, texColor3, diffuse, specular, current, tFactor, i);
            float4 alphaResult = evaluateBlendOp(alphaOp, aArg1, aArg2, current, diffuse, stageTexColor, tFactor, current);
            alphaVal = alphaResult.a;
        }
        
        current = float4(colorResult.rgb, alphaVal);
    }
    
    // --- Alpha Test ---
    if (fragUniforms.alphaTestEnable != 0) {
        if (!alphaTestPass(fragUniforms.alphaFunc, current.a, fragUniforms.alphaRef)) {
            discard_fragment();
        }
    }

    // --- Fog ---
    if (fragUniforms.fogMode != 0) {
        current.rgb = mix(fragUniforms.fogColor.rgb, current.rgb, in.fogFactor);
    }

    // Add specular only if D3DRS_SPECULARENABLE is TRUE
    if (fragUniforms.specularEnable != 0) {
        current.rgb = clamp(current.rgb + specular.rgb, 0.0, 1.0);
    }

    return current;
}
