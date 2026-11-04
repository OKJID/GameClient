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
    float4 texCoord0    [[attribute(2)]]; // texcoord sets: 1..4 floats, Metal pads to (x, y, 0, 1)
    float3 normal       [[attribute(3)]]; // vertex normal (for lighting)
    float4 specVtxColor [[attribute(4)]]; // specular vertex color (BGRA normalized)
    float4 texCoord1    [[attribute(5)]];
    float4 texCoord2    [[attribute(6)]];
    float4 texCoord3    [[attribute(7)]];
    float4 texCoord4    [[attribute(8)]];
    float4 texCoord5    [[attribute(9)]];
    float4 texCoord6    [[attribute(10)]];
    float4 texCoord7    [[attribute(11)]];
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
    
    out.texCoord0 = in.texCoord0;
    out.texCoord1 = in.texCoord1;
    out.texCoord2 = in.texCoord2;
    out.texCoord3 = in.texCoord3;
    out.texCoord4 = in.texCoord4;
    out.texCoord5 = in.texCoord5;
    out.texCoord6 = in.texCoord6;
    out.texCoord7 = in.texCoord7;
    
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
struct StageInputs {
    float4 diffuse;
    float4 specular;
    float4 current;
    float4 temp;
    float4 tFactor;
    float4 texel;
};

float4 resolveArg(uint argId, thread const StageInputs &inputs) {
    float4 val;
    switch (argId & 0xF) {
        case D3DTA_DIFFUSE:  val = inputs.diffuse;  break;
        case D3DTA_TEXTURE:  val = inputs.texel;    break;
        case D3DTA_TFACTOR:  val = inputs.tFactor;  break;
        case D3DTA_SPECULAR: val = inputs.specular; break;
        case D3DTA_TEMP:     val = inputs.temp;     break;
        default:             val = inputs.current;  break;
    }

    if (argId & D3DTA_COMPLEMENT)     val = 1.0 - val;
    if (argId & D3DTA_ALPHAREPLICATE) val = float4(val.a);
    return val;
}

bool isTextureArg(uint argId) {
    return (argId & 0xF) == D3DTA_TEXTURE;
}

bool isActiveOp(uint op) {
    return op != 0 && op != D3DTOP_DISABLE;
}

bool isBumpOp(uint op) {
    return op == D3DTOP_BUMPENVMAP || op == D3DTOP_BUMPENVMAPLUMINANCE;
}

bool readsArg0(uint op) {
    return op == D3DTOP_MULTIPLYADD || op == D3DTOP_LERP;
}

bool readsMissingTexture(uint op, uint arg0, uint arg1, uint arg2, uint hasTexture) {
    if (hasTexture != 0) {
        return false;
    }
    if (isTextureArg(arg1) && op != D3DTOP_SELECTARG2) {
        return true;
    }
    if (isTextureArg(arg2) && op != D3DTOP_SELECTARG1) {
        return true;
    }
    return isTextureArg(arg0) && readsArg0(op);
}

// ─────────────────────────────────────────────────────
//  Stage 7: TSS Operation Evaluator
//  Computes result for both color (.rgb) and alpha (.a)
// ─────────────────────────────────────────────────────
float4 evaluateOp(uint op, float4 arg0, float4 arg1, float4 arg2, thread const StageInputs &inputs) {
    switch (op) {
        case D3DTOP_SELECTARG1:                return arg1;
        case D3DTOP_SELECTARG2:                return arg2;
        case D3DTOP_MODULATE:                  return arg1 * arg2;
        case D3DTOP_MODULATE2X:                return arg1 * arg2 * 2.0;
        case D3DTOP_MODULATE4X:                return arg1 * arg2 * 4.0;
        case D3DTOP_ADD:                       return arg1 + arg2;
        case D3DTOP_ADDSIGNED:                 return arg1 + arg2 - 0.5;
        case D3DTOP_ADDSIGNED2X:               return (arg1 + arg2 - 0.5) * 2.0;
        case D3DTOP_SUBTRACT:                  return arg1 - arg2;
        case D3DTOP_ADDSMOOTH:                 return arg1 + arg2 - arg1 * arg2;
        case D3DTOP_BLENDDIFFUSEALPHA:         return mix(arg2, arg1, inputs.diffuse.a);
        case D3DTOP_BLENDTEXTUREALPHA:         return mix(arg2, arg1, inputs.texel.a);
        case D3DTOP_BLENDFACTORALPHA:          return mix(arg2, arg1, inputs.tFactor.a);
        case D3DTOP_BLENDTEXTUREALPHAPM:       return arg1 + arg2 * (1.0 - inputs.texel.a);
        case D3DTOP_BLENDCURRENTALPHA:         return mix(arg2, arg1, inputs.current.a);
        case D3DTOP_MODULATEALPHA_ADDCOLOR:    return float4(arg1.rgb + arg1.a * arg2.rgb, arg1.a);
        case D3DTOP_MODULATECOLOR_ADDALPHA:    return float4(arg1.rgb * arg2.rgb + arg1.a, arg1.a);
        case D3DTOP_MODULATEINVALPHA_ADDCOLOR: return float4((1.0 - arg1.a) * arg2.rgb + arg1.rgb, arg1.a);
        case D3DTOP_MODULATEINVCOLOR_ADDALPHA: return float4((1.0 - arg1.rgb) * arg2.rgb + arg1.a, arg1.a);
        case D3DTOP_BUMPENVMAP:                return inputs.current;
        case D3DTOP_BUMPENVMAPLUMINANCE:       return inputs.current;
        case D3DTOP_DOTPRODUCT3:               return float4(dot(arg1.rgb - 0.5, arg2.rgb - 0.5) * 4.0);
        case D3DTOP_MULTIPLYADD:               return arg0 + arg1 * arg2;
        case D3DTOP_LERP:                      return arg0 * arg1 + (1.0 - arg0) * arg2;
        default:                               return arg1 * arg2;
    }
}

float4 evaluateStageOp(uint op, uint arg0, uint arg1, uint arg2, uint hasTexture, thread const StageInputs &inputs) {
    if (readsMissingTexture(op, arg0, arg1, arg2, hasTexture)) {
        return inputs.current;
    }
    return evaluateOp(op, resolveArg(arg0, inputs), resolveArg(arg1, inputs), resolveArg(arg2, inputs), inputs);
}

float4 combineStage(constant TextureStageConfig &stage, uint hasTexture, thread const StageInputs &inputs) {
    float4 color = evaluateStageOp(stage.colorOp, stage.colorArg0, stage.colorArg1, stage.colorArg2, hasTexture, inputs);
    if (stage.colorOp == D3DTOP_DOTPRODUCT3) {
        return saturate(color);
    }
    if (!isActiveOp(stage.alphaOp)) {
        return float4(saturate(color.rgb), inputs.current.a);
    }
    float4 alpha = evaluateStageOp(stage.alphaOp, stage.alphaArg0, stage.alphaArg1, stage.alphaArg2, hasTexture, inputs);
    return saturate(float4(color.rgb, alpha.a));
}

// ─────────────────────────────────────────────────────
//  Stage 7: Bump environment mapping
//  A BUMPENVMAP stage offsets the texcoords of the next stage
// ─────────────────────────────────────────────────────
struct BumpState {
    bool active;
    bool luminance;
    float2 offset;
    float luminanceScale;
};

BumpState bumpStateForNextStage(constant TextureStageConfig &stage, float4 bumpTexel) {
    float4 m = stage.bumpEnvMatrix;
    BumpState bump;
    bump.active = true;
    bump.luminance = stage.colorOp == D3DTOP_BUMPENVMAPLUMINANCE;
    bump.offset = float2(m.x * bumpTexel.x + m.z * bumpTexel.y, m.y * bumpTexel.x + m.w * bumpTexel.y);
    bump.luminanceScale = saturate(bumpTexel.z * stage.bumpEnvLScale + stage.bumpEnvLOffset);
    return bump;
}

float4 sampleStageTexture(uint stage, float4 uv, constant FragmentUniforms &fragUniforms,
                          array<texture2d<float>, 8> textures, array<texturecube<float>, 8> cubeTextures,
                          array<sampler, 8> samplers) {
    uint kind = fragUniforms.hasTexture[stage];
    if (kind == 0) {
        return float4(0.0, 0.0, 0.0, 1.0);
    }
    float lodBias = fragUniforms.stages[stage].lodBias;
    float4 texel = kind == 2 ? cubeTextures[stage].sample(samplers[stage], uv.xyz, bias(lodBias))
                             : textures[stage].sample(samplers[stage], uv.xy, bias(lodBias));
    return unpackTexelFormat(texel, fragUniforms.texFormatType[stage]);
}

// ─────────────────────────────────────────────────────
//  Stage 7: Fragment Shader with full TSS support
// ─────────────────────────────────────────────────────
fragment float4 fragment_main(FragmentIn stageIn [[stage_in]],
                             constant Uniforms &uniforms [[buffer(1)]],
                             constant FragmentUniforms &fragUniforms [[buffer(2)]],
                             array<texture2d<float>, 8> textures [[texture(0)]],
                             array<texturecube<float>, 8> cubeTextures [[texture(8)]],
                             array<sampler, 8> samplers [[sampler(0)]],
                             float2 pointCoord [[point_coord]]) {
    // D3D generates per-sprite UVs in the rasterizer when POINTSPRITEENABLE is
    // on; Metal exposes them as point_coord. Substituting into a local copy
    // keeps every downstream stage untouched. point_coord is undefined for
    // non-point primitives, hence the gate.
    FragmentIn in = stageIn;
    if (uniforms.pointSpriteEnable != 0) {
        applyPointSpriteTexCoords(in, pointCoord);
    }

    StageInputs inputs;
    inputs.diffuse = in.color;
    inputs.specular = in.specularColor;
    inputs.current = in.color;
    inputs.temp = float4(0.0);
    inputs.tFactor = fragUniforms.textureFactor;

    BumpState bump;
    bump.active = false;

    for (uint i = 0; i < 8; i++) {
        constant TextureStageConfig &stage = fragUniforms.stages[i];
        if (!isActiveOp(stage.colorOp)) break;

        bool clipped;
        float4 uv = computeTexCoord(uniforms, in, fragUniforms.texCoordIndex[i], i, clipped);
        if (clipped) {
            discard_fragment();
        }
        if (bump.active) {
            uv.xy += bump.offset;
        }

        inputs.texel = sampleStageTexture(i, uv, fragUniforms, textures, cubeTextures, samplers);
        if (bump.active && bump.luminance) {
            inputs.texel *= bump.luminanceScale;
        }

        float4 result = combineStage(stage, fragUniforms.hasTexture[i], inputs);
        bump.active = isBumpOp(stage.colorOp) && fragUniforms.hasTexture[i] != 0;
        if (bump.active) {
            bump = bumpStateForNextStage(stage, inputs.texel);
        }

        if (stage.resultArg == D3DTA_TEMP) {
            inputs.temp = result;
        } else {
            inputs.current = result;
        }
    }

    float4 color = inputs.current;
    if (fragUniforms.specularEnable != 0) {
        color.rgb = saturate(color.rgb + inputs.specular.rgb);
    }

    if (fragUniforms.alphaTestEnable != 0 && !alphaTestPass(fragUniforms.alphaFunc, color.a, fragUniforms.alphaRef)) {
        discard_fragment();
    }

    if (fragUniforms.fogMode != 0) {
        color.rgb = mix(fragUniforms.fogColor.rgb, color.rgb, in.fogFactor);
    }

    return color;
}
