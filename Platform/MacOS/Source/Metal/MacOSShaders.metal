#include <metal_stdlib>
using namespace metal;

// Uniforms, FragmentUniforms, VertexOut/FragmentIn and the helpers that shaders
// compiled at run time share with this file (ShaderTranslator wrappers).
#include "MacOSShaderInterface.h"

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

// ─────────────────────────────────────────────────────
//  Vertex Shader (with DX8 per-vertex lighting)
// ─────────────────────────────────────────────────────

vertex VertexOut vertex_main(VertexIn in [[stage_in]],
                            constant Uniforms &uniforms [[buffer(1)]],
                            constant LightingUniforms &lighting [[buffer(3)]]) {
    VertexOut out;
    // Metal rejects a point_size of 0; every early-return path must carry a
    // valid value even though only POINTLIST draws consume it.
    out.pointSize = 1.0;

    float4 pos = float4(in.position.xyz, 1.0);
    
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

    // Point sprite size, per DX8 rules:
    //   POINTSCALEENABLE off → size is already in screen pixels
    //   on  → size = viewportHeight * POINTSIZE * sqrt(1 / (A + B*D + C*D²))
    // with D the eye-space distance to the vertex. Result is clamped to
    // [POINTSIZE_MIN, POINTSIZE_MAX].
    if (uniforms.pointSpriteEnable != 0) {
        float pointPixels = uniforms.pointSize;
        if (uniforms.pointScaleEnable != 0) {
            float d = length(camPos.xyz);
            float denom = uniforms.pointScaleA
                        + uniforms.pointScaleB * d
                        + uniforms.pointScaleC * d * d;
            pointPixels = uniforms.screenSize.y * uniforms.pointSize
                        * sqrt(1.0 / max(denom, 1e-6));
        }
        out.pointSize = clamp(pointPixels, uniforms.pointSizeMin, uniforms.pointSizeMax);
    }

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
        applyClipPlanes(uniforms, uniforms.world * pos, out);
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
    
    applyClipPlanes(uniforms, uniforms.world * pos, out);
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

bool isTextureArg(uint argId) {
    return (argId & 0xF) == D3DTA_TEXTURE;
}

bool isActiveOp(uint op) {
    return op != 0 && op != D3DTOP_DISABLE;
}

bool opSamplesTexture(uint op, uint arg0, uint arg1, uint arg2) {
    if (!isActiveOp(op)) {
        return false;
    }

    if (op == D3DTOP_BLENDTEXTUREALPHA) {
        return true;
    }

    const bool readsArg0 = op == D3DTOP_MULTIPLYADD || op == D3DTOP_LERP;
    const bool readsArg1 = op != D3DTOP_SELECTARG2;
    const bool readsArg2 = op != D3DTOP_SELECTARG1;

    return (readsArg0 && isTextureArg(arg0)) || (readsArg1 && isTextureArg(arg1)) || (readsArg2 && isTextureArg(arg2));
}

bool stageSamplesMissingTexture(constant TextureStageConfig &stage, uint hasTexture) {
    if (hasTexture != 0) {
        return false;
    }

    return opSamplesTexture(stage.colorOp, stage.colorArg0, stage.colorArg1, stage.colorArg2)
        || opSamplesTexture(stage.alphaOp, D3DTA_CURRENT, stage.alphaArg1, stage.alphaArg2);
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
//  Stage 7: Fragment Shader with full TSS support
// ─────────────────────────────────────────────────────
fragment float4 fragment_main(FragmentIn stageIn [[stage_in]],
                             constant Uniforms &uniforms [[buffer(1)]],
                             constant FragmentUniforms &fragUniforms [[buffer(2)]],
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
                             sampler sampler3 [[sampler(3)]],
                             float2 pointCoord [[point_coord]]) {
    // D3D generates per-sprite UVs in the rasterizer when POINTSPRITEENABLE is
    // on; Metal exposes them as point_coord. Substituting into a local copy
    // keeps every downstream stage untouched. point_coord is undefined for
    // non-point primitives, hence the gate.
    FragmentIn in = stageIn;
    if (uniforms.pointSpriteEnable != 0) {
        in.texCoord = pointCoord;
        in.texCoord2 = pointCoord;
    }

    // ════════════════════════════════════════════════════
    //  TSS (Texture Stage State) path — fallback when no PS active
    // ════════════════════════════════════════════════════
    
    float3 uv0 = computeTexCoord(uniforms, in, fragUniforms.texCoordIndex[0], 0);

    float3 uv1 = computeTexCoord(uniforms, in, fragUniforms.texCoordIndex[1], 1);
    float3 uv2 = computeTexCoord(uniforms, in, fragUniforms.texCoordIndex[2], 2);
    float3 uv3 = computeTexCoord(uniforms, in, fragUniforms.texCoordIndex[3], 3);
    
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
        if (i > 0 && stageSamplesMissingTexture(fragUniforms.stages[i], fragUniforms.hasTexture[i])) continue;

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
