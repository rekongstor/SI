// ---------------------------------------------------------------------------
//
// This file defines constant register mappings for all shaders
// 
// ----------------------------------------------------------------------------
#ifndef REGISTERS_COMMON_FX_INCLUDED
#define REGISTERS_COMMON_FX_INCLUDED

#include "sys_resource_defines.hlsl"

#define _MAKE_S_REG_INDEX(ID)          s##ID

//#define SAMPLE_LEVEL_OBJECT(obj, ID, uv, lod)    obj.SampleLevel (PS_SAMPLERS[PS_##ID##_SMP], uv, lod)



#define LWI_BIT                                 (1 << 31)
#define CSII_STATIC_INST_DATA_BLOCK_SIZE_INST   4          
#define CSII_STATIC_INST_DATA_BLOCK_SIZE_F4     5          
#define CSII_STATIC_DEBUG_DATA_SIZE_U1          2          // 64b of instUID
                                                      

// ----------------------------------------------------------------------------
// Debug visualisation constants
// ----------------------------------------------------------------------------
#define DBGVIS_NONE               0
#define DBGVIS_ALBEDO             1
#define DBGVIS_GBUF_NORM_PIX      2
#define DBGVIS_GBUF_GLOSS         3
#define DBGVIS_GBUF_MET           4
#define DBGVIS_GBUF_LOCAL_AO      5
#define DBGVIS_LAYERS_COUNT       6
#define DBGVIS_DYN_LIGHTS_COUNT   7
#define DBGVIS_REFLECTIONS_COUNT  8
#define DBGVIS_LIGHTMAP_REFL_K    9
#define DBGVIS_GLOBAL_WIND_POWER  10

struct WIND_DATA {
   #ifdef __cplusplus
      float leafs_noise[4];         // x-amplitude, y-speed, z-detail, w-wind_amplitude_impact
      float pivots_data[4];         // xyz - max pivot val, w - max hierarchy len val
      float hierarchy_thickness[4]; // thickness of different lvl-s in hierarchy
      float hierarchy_wind_mult[4]; // wind multiply of different lvl-s in hierarchy
      float wind_lag_speed[3];      // wind lag speed by x, y, z
      float fake_inertia_speed;     // speed of fake inertia
      float wind_lag_power[3];      // wind lag power by x, y, z
      float max_tree_len;           // max len of 1 hierarchy lvl
   #else
      float4 leafs_noise;
      float4 pivots_data;
      float4 hierarchy_thickness;
      float4 hierarchy_wind_mult;
      float3 wind_lag_speed;
      float fake_inertia_speed;
      float3 wind_lag_power;
      float max_tree_len;
   #endif
};

// struct with tpl info for TPL_COMMON_DATA_BUF buffer
struct TPL_COMMON_DATA {
   WIND_DATA wind_data;
   float bboxDepth;
   int diceNum;
};


#define WIND_FLAG_OFF_WIND          0x01
#define WIND_FLAG_APPLY_REDUCE      0x02
#define WIND_FLAG_OFF_HIERARCHY     0x04
#define WIND_FLAG_OFF_NON_HIERARCHY 0x08


// ----------------------------------------------------------------------------
//
// CONSTANT BUFFERS
//
// ----------------------------------------------------------------------------



//
// COMMON PER RenderFrame() CONSTANTS. MUST BE ALIGNED BY 4!!!
//
// Dynamic light parameters (4 x float4 for each light):
//   direct/point/spot:
//     const0: xyz = world-space position;       w = position coefficient;
//     const1: xyz = color;                      w = angular attenuation coefficient #1;
//     const2: xyz = world-space spot direction; w = angular attenuation coefficient #2;
//     const3: xyz = distance attenuation coefficients; w = on/off switch
//
// Setup of these coefficients for particular light type is specially handled in 
// light.cpp and vid_pass.cpp engine modules
//
// Setup in vidSetupDynLightConsts()
#define SM_LIGHT_COUNT_MAX        4
#define DYN_LIGHTS_REG_PER_LIGHT  4
#define DYN_LIGHTS_TEX_MATR_SIZE  4
#define DYN_LIGHTS_PER_TILE      64
#define LBUF_NUM_Z_SLICES        64u
#define DEPTH_Z_TILE_SIZE        32u          // depth & capsule shadows  & lbuf mask tile size
#define PS_SH_NUM_REGS           SH_NUM_REGS  // common size for SH coeff set

#define SH_NUM_REGS                       8
// VS constants
#define VS_SH_NUM_REGS                    SH_NUM_REGS  // common size for SH coeff set
// Fog common constants
#define FOG_REG_STATE_COLOR               1
#define FOG_REG_STATE_ADDITIVE            2
#define FOG_REG_STATE_DENX_GAUSS          4
#define FOG_REG_STATE_DENZ_GAUSS          8
#define FOG_REG_STATE_DENY_GAUSS         16
#define FOG_REG_STATE_DENY_SQR           32
#define MAX_VELOCITY_PIXELS              REG_COMMON_MB_PARAMS[0].x

#define MASK_BIT_MOTION_BLUR_MASK        0x01
#define MASK_BIT_TRANSP_OUTLINE          0x02
#define MASK_BIT_ANIM                    0x04
#define MASK_BITS_BLUR_SHADOW_0          0x08
#define MASK_BITS_BLUR_SHADOW_1          0x10
#define MASK_BIT_TRANSLUCENT             0x20

#define MAX_SHAPE_COEFS   256

#define REFLECTIONS_CUBEMAP_SIZE           6

BEGIN_REFLECTION_TABLE_COMMON
// Common constant buffer
// * This constant buffer is used for per-camera update
// Setup in rendSYSTEM::PrepareSharedConstantBuffer()
CONST_BUFFER_BEGIN_USAGE(CB_COMMON, 0, PER_FRAME_SET, USAGE_PS|USAGE_VS|USAGE_DS|USAGE_HS|USAGE_CS, BINDING_NO_OFFSET)
   // Setup in rendFOG_RENDER::SetupFogConstants
   DECLARE_CONST(REG_COMMON_FOG_PARAMS,                 2); // x = fogDensityAtCamera, y = atmosphereScale, z = densityOffset, w = atmosphereCutoffDist
   DECLARE_CONST(REG_COMMON_FOG_COLOR,                  1); // xyz = primary fog color; w = p3 in RayleighFactor
   DECLARE_CONST(REG_COMMON_FOG_SUN_DIR,                1); // xyz = sun dir;           w = p1 in RayleighFactor
   DECLARE_CONST(REG_COMMON_FOG_RAYLEIGH_FACTOR,        1); // xyz = second color;      w = p2 in RayleighFactor
   DECLARE_CONST(REG_COMMON_FOG_FROXEL_PARAMS,              1); // x - froxel_z_near, y - foxel_z_far, z - rangeKa, w - rangeKb (ka & kb are froxel grid constants)
   // Two param sets of fog
   DECLARE_CONST(PS_REG_COMMON_FOG_PLANE_MIRROR,        2); // xyz = primary fog color; w = p3 in RayleighFactor
   DECLARE_CONST(PS_REG_COMMON_FOG_ATMOSPHERE_0,       12);
   DECLARE_CONST(PS_REG_COMMON_FOG_ATMOSPHERE_EXTRA,    2);
      
   // Setup in rendHDR_SYSTEM::SetupShaderConstants()
   DECLARE_CONST(PS_REG_COMMON_HDR_PARAMS,              2); // 0:
                                                            // 1: x - average luminance for current color camera
   
   // Setup in rendSYSTEM::PrepareSharedConstantBuffer
   DECLARE_CONST(PS_REG_COMMON_ELAPSED_TIME,            1); // x - elapsed time level, y used for debug mip visualization, z - elapsed time, w - cur frame nmb
   DECLARE_CONST(PS_REG_COMMON_DEBUG_SHOW_LIGHTING,     6); // see .CPP setup code for content description
   DECLARE_CONST(PS_REG_COMMON_AMBIENT,                 1); // 1, xyz = additive RGB, w = unused   
   DECLARE_CONST(COMMON_GAMMA_PARAMS,                   1); // x - gamma, y - contrast, z - brightness, w - unused
   DECLARE_CONST(REG_COMMON_MB_PARAMS,                  1); // x - maximum velocity in pixels (used to scale velocity in R8G8 buffer)
   
   // Setup in rendSYSTEM::PrepareSharedConstantBuffer
   DECLARE_CONST(COMMON_REND_SCENE_PARAMS,              2); // 0: x - enable dithering in imposter lods, y - is terrain rendering enable, z - enable gbuf norm blend
                                                            // 1: x - dbg_smCacheBleedRedGlob, y - outline min occl depth
   
   // SM COMMON CONSTANTS
   // Setup in rendSHADOW_MAP_SYSTEM::SetupSMArrayConstants
   DECLARE_CONSTI(COMMON_SM_NUM_SPLITS,                 4); // SM_LIGHT_COUNT_MAX; x = num splits; y = static split idx; z = fps model split idx
   DECLARE_CONST (COMMON_SM_MATR_ARRAY,               112); // SM_LIGHT_COUNT_MAX * 7 * 4; projection matr from world space to light space ([0;1]x[0;1])
   DECLARE_CONST (COMMON_SM_FITK_ARRAY,                28); // SM_LIGHT_COUNT_MAX * 7; x/y = min/max values for local light space coords
   DECLARE_CONST (COMMON_SM_SPLIT_POS,                 28); // SM_LIGHT_COUNT_MAX * 7; 7 <=> SM_MAX_ARRAY_COL
   
   // LIGHTING COMMON
   // Setup in vidSetupForwardLightConsts
   DECLARE_CONSTI(COMMON_LIGHTS_NUM,                    1); // 1  x == numSMLights; y == numDynLights; z == numForceDynLights; w == numForceSMLights (newly created lights in Maya)
   DECLARE_CONST (COMMON_DYN_LIGHT_PARAMS,            144); // (32 + SM_LIGHT_COUNT_MAX) * DYN_LIGHTS_REG_PER_LIGHT == 272
   
   DECLARE_CONST (COMMON_ESM_SPOT_MATR,               1024); // MAX_ESM_SPOT_LIGHTS(256) * 4 == 1024
   DECLARE_CONST (COMMON_ESM_POINT_CONSTS,            896); // MAX_ESM_POINT_LIGHTS(128) * (6 + 1) == 896
   
   DECLARE_CONST (COMMON_DYN_TEX_PROJ_MATR,           144); // (32 + SM_LIGHT_COUNT_MAX) * 4 == 144
   DECLARE_CONST (COMMON_DYN_TEX_IDX,                  36); // 32 + SM_LIGHT_COUNT_MAX, texture index in array
   DECLARE_CONST (COMMON_LM_INTENSITY,                  1); // 1  x == numSMLights y == numDynLights z = SM cache min variance
   
   DECLARE_CONST (COMMON_LM_3D,                         4); //
   
   // REFLECTIONS
   // Setup in rendSYSTEM::ScreenReflectionsSetupConstants
   DECLARE_CONST (PS_REG_REFLECTIONS_MATRVIEW,          3);// TODO: Delete, use COMMON_VIEW_MATRIX
   DECLARE_CONST (PS_REG_REFLECTIONS_MATRVIEW_INV,      3);
   DECLARE_CONST (PS_REG_REFLECTIONS_MATRVIEW_PREV,     3);
   DECLARE_CONST (PS_REG_REFLECTIONS_JITTER_PARAMS,     1);// xy = jitter offset in pixel; z = int jitter sample idx; w = free
   DECLARE_CONST (PS_REG_REFLECTIONS_CAM_PARAMS,        1);
   DECLARE_CONST (PS_REG_REFLECTIONS_PARAMS,            2);
   DECLARE_CONST (PS_REG_REFLECTIONS_DEBUG_PARAMS,      1);
   DECLARE_CONST (PS_REG_REFLECTIONS_CUBEMAPS,       1530); // 255 cubemaps * REFLECTIONS_CUBEMAP_SIZE
   
   DECLARE_CONST (COMMON_VIEWPROJ_MATRIX_NO_JIT,        4); // camera view projection
   DECLARE_CONST (COMMON_VIEWPROJ_MATRIX_PREV,          4); // camera view projection prev frame (CURRENT frame jitter)
   
   // WIND SYSTEM
   DECLARE_CONST (COMMON_WIND_PARAMS,                   4); // 0: x - enabled, y - power, z- speed, w - detail
                                                            // 1: xyz - wind direction, w - contrast 
                                                            // 2: debug info for hierarchy wind animation
                                                            // 3: wind reduce scale\bias\gamma
   // SSAO
   // Setup in rendSSAO_SYSTEM::SetupShaderConstants
   DECLARE_CONST (PS_REG_SSAO_COMMON_PARAMS, 1); // x == gamma, y == ambient, z == use bent norm coeff
   DECLARE_CONST (PS_REG_FAKE_LIGHT_COMMON_PARAMS, 1);
   DECLARE_CONST (PS_REG_COMMON_WPN_ZSCALE,  1);
   DECLARE_CONST (PS_REG_COMMON_SKIN_BENT_NORMAL_FACTORS, 5);
   
   // Terrain and its clipmaps
   DECLARE_CONST (COMMON_TERRAIN_WORLD_OBJ_MATRIX,             3);
   DECLARE_CONST (COMMON_TERRAIN_OBJ_WORLD_MATRIX,             3);
   DECLARE_CONST (COMMON_TERRAIN_PARAMS,                       1);
   DECLARE_CONST (COMMON_TERRAIN_CLIPMAPS_PARAMS,              2);
   DECLARE_CONST (COMMON_TERRAIN_CLIPMAP_ORIGIN,              16); // 32 (limits number of possible clipmaps) pairs (origin.x, origin.z)
   DECLARE_CONST (COMMON_TERRAIN_CLIPMAP_ORIGIN_TORUS_OFFSET, 16);
CONST_BUFFER_END;


// combine sets
// Buffer is accessed via vidDRIVER_INTERFACE::BeginCommonShaderConst()
// * This constant buffer is updated several times per camera (but not per-dip)
// Setup in:
// * rendSYSTEM::SetupInitialState
// * vidPassBeginCamera
CONST_BUFFER_BEGIN_USAGE(CB_COMMON_DYN, 1, PER_FRAME_SET, USAGE_VS|USAGE_PS|USAGE_DS|USAGE_CS|USAGE_HS, BINDING_DYNAMIC_OFFSET)
   // rendLBUF_CLUSTERED::EnsureGPUConst
   DECLARE_CONST(COMMON_LBUF_PARAMS,                    1); // xy - rtSize.xy, z - rangeKa, w - rangeKb (ka & kb are froxel grid constants)
   DECLARE_CONSTI(PS_REG_REFLECTIONS_NELEM,             1); // xyz = free; w = num cubemaps
   DECLARE_CONSTI(PS_REG_COMMON_FOG_PARAM_SET,          1); // x - idx of parameter set for fog
   
   DECLARE_CONST(COMMON_VIEW_POSITION,                  1); // camera view position
   DECLARE_CONST(COMMON_VIEW_POSITION_PREV,             1); // camera view prev position
   DECLARE_CONST(COMMON_VIEWPROJ_MATRIX,                4); // camera view projection
   DECLARE_CONST(COMMON_FPMODEL_VIEWPROJ_MATRIX,        4); // camera view projection for first person model (it has different FOV and different z-scale)
   DECLARE_CONST(COMMON_FPMODEL_ZSCALE,                 1); // first person model z-scale
   DECLARE_CONST(COMMON_VIEW_MATRIX,                    3); // camera view matrix (for billboards + lights)
   DECLARE_CONST(COMMON_FPMODEL_CORRECTION_MATRIX,      4); // convert world position from fp to original
   DECLARE_CONST(COMMON_PROJ_MATRIX,                    4); // camera projection matrix
CONST_BUFFER_END;

CONST_BUFFER_BEGIN_USAGE(CB_SCREEN_RECT_DATA, 2, PER_FRAME_SET, USAGE_VS|USAGE_PS|USAGE_HS|USAGE_CS, BINDING_DYNAMIC_OFFSET)
   // Setup in UpdateRenderTargetAndRectSize()
   DECLARE_CONST(COMMON_VP_PARAMS,                      1); // xy = (1 / RTSize.xy); zw = (0.5 / RTSize.xy)
   DECLARE_CONST(SCREEN_RECT,                           1); // Screen rectangle parameters: xy - left-bottom cornet, zw - width and height
   DECLARE_CONST(SCREEN_UV,                             1); // Screen rectangle textrue coords: xy - left-bottom corner, zw - width and height
   DECLARE_CONST(RECT_DEPTH,                            1); // Screen rect Z-pos (x - z-value, yzw - FREE)
   DECLARE_CONST(LWI_PARAMS,                            2); // 1 - Lwi dip params (x - start instance index, y - objDataOffset, zw - transitionData)
                                                            // 2 - inst data layout (x - instSizeF4 | skinInfoOffs, y - bonesMatrOffs | prev, z - quatOffs | prev)
CONST_BUFFER_END;

// ----------------------------------------------------------------------------
//
//  Slots 3 and 4 are reserved to per pass static and dynamic constant buffers
//
//  #define STATIC_CB_REGISTER  3
//  #define DYNAMIC_CB_REGISTER 4
//
// ----------------------------------------------------------------------------

// all per draw below
INSTANCED_CONST_BUFFER_BEGIN_USAGE(CB_PIX_OBJ, 5, PER_DRAW_CB_SET, USAGE_VS|USAGE_PS, BINDING_DYNAMIC_OFFSET)
   DECLARE_CONST(VS_REG_COMMON_OBJ_TEX_OFFSET,          1);// xy = texOffs0, zw = texOffs1
   DECLARE_CONST(VS_REG_COMMON_OBJ_WORLD_MATRIX,        3);// for dual quaternions we store special data here
   DECLARE_CONST(VS_REG_COMMON_OBJ_WORLD_MATRIX_PREV,   3);// for dual quaternions we store special data here, prev. frame
   DECLARE_CONST(PS_REG_COMMON_ALPHA_KILL_REF,          1);// x = akillRef, (y = min, z = scale, w = bias) for adaptive akill
   DECLARE_CONST(PS_REG_COMMON_OBJ_SKIN_OFFSET,         1); // Offset in skinning buffer: x = skin offset, y = skin data offset, z = skin offset for previous frame bones, w = hasVelocity ? 0.f : 1.f
   
   DECLARE_CONST(PS_REG_COMMON_OBJ_NEGATIVE_SCALE,      1);// x = transform matrix for object has negative determinant; y = anim inst obbOrg.y, z = 1 / obbSize.y (not valid for static geometry)
                                                                                                                                                                                                                                       // x - transform (matrices or quaternions) offset, y - data offset, zw - unused
   DECLARE_CONSTI(PS_REG_COMMON_OBJ_CUBE_MASK,          1); // x - custom cubemap mask, y = instId, z - obj id, w - split id
   DECLARE_CONSTI(PS_REG_COMMON_OBJ_CAUST_REFR_IDX,     1); // x = caustic volume idx; y = refraction cubemap idx
   
   DECLARE_CONST(PS_REG_COMMON_ZFILL_VC_MASK,           2); // 0 : xyzw = mask, 1 : x - use mask

   DECLARE_CONSTI(PS_REG_COMMON_OBJ_DECAL_NUM,          1); // number of decals
   DECLARE_CONSTI(PS_REG_COMMON_OBJ_DECAL_IDX,          4); // decal indexes
   DECLARE_CONSTI(PS_REG_COMMON_VERTEX_MASKING_FLAGS,   1); // x - masking flags, y - second weights, z - motion blur mask, w - dissolve on/off
   DECLARE_CONST(VS_REG_COMMON_OBJ_OBB,                 1); // xyz - anim inst center (not valid for static geometry), w - position-based hash for vertex animation
   DECLARE_CONSTI(PS_REG_COMMON_TPL_IDX,                1); // x - tpl idx for TPL_COMMON_DATA_BUF buffer
                                                            // y - debug probe visual
   
   DECLARE_CONST(VS_REG_COMMON_VERTEX_ANIM_CONTROL,     2); // [0]: x - startFrame, y - numFrames (>=0), z - frameOffset (is used to pause/resume animation), w - begingTime
                                                            // [1]: x - normalized playback speed in (fps/numFrames), y (0th bit) - enable vertex anim control, y (1st bit) - loop animation,
                                                            // [1]: y (2nd bit) - inverse animation
INSTANCED_CONST_BUFFER_END(CB_PIX_OBJ, 5, PER_DRAW_CB_SET);


// Constant per-object material data
// Filled in bool vidPrerecordSplitConstantBufferMaterial()
CONST_BUFFER_BEGIN_USAGE(CB_OBJ_MATERIAL_DATA, 6, PER_DRAW_CB_SET, USAGE_PS|USAGE_VS, BINDING_NO_OFFSET)
   DECLARE_CONSTI(OBJ_GLT_UVSET_INDICES,               1); // x = layer0, y = layer1, z = layer2, w = free
   DECLARE_CONST (OBJ_LAYER_TILING,                    2); // 0 : x = base.u; y = layer0.u; z = layer1.u; w = layer2.u
   DECLARE_CONST (OBJ_LAYER_TILING_BASE_INV,           1); // x = 1 / base.u; y = 1 / base.v; zw = free
   DECLARE_CONST (OBJ_GLT_TRANSP_VC_MASK,              2); // 0 : xyzw - dp - mask, 1: x - is mask used
   DECLARE_CONST (OBJ_MTLBLEND_LAYER_CONST,           52); // MTL_LAYER_CONST_OFFS_LAST * PS_GLT_MAX_MTL_LAYERS
   DECLARE_CONST (OBJ_REFLECTION_PARAMS,               1); // mtlDesc.<base,layer0,layer1,layer2>.params.refIntensity
   DECLARE_CONST (OBJ_PARALLAX_SECOND_VC_SWZ,          1);
   DECLARE_CONST (OBJ_PARALLAX_FLATTEN_VC_SWZ,         1);
   // !!! DS: ACHTUNG: following 3 constants are prerecorded in separate method vidPrerecordSplitConstantBufferCompression()
   // If you want to add something material related - add before these constants (sure, you can kill author as well)
   DECLARE_CONST(OBJ_COMPR_VERT_OFFSET,                1);
   DECLARE_CONST(OBJ_COMPR_VERT_SCALE,                 1);
   DECLARE_CONST(OBJ_COMPR_TEX,                        2);
CONST_BUFFER_END;

// Shapes
CONST_BUFFER_BEGIN_USAGE(CB_VS_SHAPES, 7, PER_DRAW_CB_SET, USAGE_VS, BINDING_DYNAMIC_OFFSET)
   DECLARE_CONST(VS_REG_SHAPES, 64); // MAX_SHAPE_COEFS/4
CONST_BUFFER_END;

//
// pass specific constant buffers
//
// ----------------------------------------------------------------------------
#define CAUSTICS_PARAMS              0 // xyzw - intensities
#define CAUSTICS_TILING              1 // xy - tiling, z - falloff, w - anim speed
#define CAUSTICS_LIGHT_MATRIX        2 // xyzw - light matrices (2 x 3)
#define CAUSTICS_LIGHT_COLOR         8 // xyzw - light colors (2)
#define CAUSTICS_START              10 // x - start plane, y - end plane
#define CAUSTICS_DATA_OFFSET        11

//glt - caustics
CONST_BUFFER_BEGIN_USAGE(CB_CAUSTICS_DATA, 8, PER_DRAW_CB_SET, USAGE_PS, BINDING_NO_OFFSET)
   DECLARE_CONST(CAUSTICS_VOLUME_PLANES, 400);
   DECLARE_CONST(CAUSTICS_DATA,          920);
CONST_BUFFER_END;
   
//glt - decal
CONST_BUFFER_BEGIN_USAGE(CB_DECAL_TYPE, 9, PER_DRAW_CB_SET, USAGE_PS, BINDING_NO_OFFSET)
   DECLARE_CONST(DECAL_TYPE_DATA, 4096);
CONST_BUFFER_END;


// ----------------------------------------------------------------------------
//
// Common Textures binding
//
// ----------------------------------------------------------------------------
DECLARE_TEXTURE_POOL(0, 512)

DECLARE_TEX_2D(COMMON_FONT,                      63, PER_FRAME_SET, 1, PS_SMP_CLAMP_LINEAR, 0);
DECLARE_TEX_2D(COMMON_SSAO_MASK,                 64, PER_FRAME_SET, 1, PS_SMP_CLAMP_POINT,  0);
DECLARE_TEX_BUF(uint,  COMMON_SH_INDICES,        65, PER_FRAME_SET);
DECLARE_TEX_2D(COMMON_SM_ARRAY,                  66, PER_FRAME_SET, 1, PS_SMP_CLAMP_CMP,    0);
DECLARE_TEX_2D(COMMON_SM_MASK0,                  67, PER_FRAME_SET, 1, PS_SMP_CLAMP_POINT,  0);
//DECLARE_TEX_2D(COMMON_SM_MASK1,                  68, PER_FRAME_SET, 1, PS_SMP_CLAMP_POINT,  0);
DECLARE_TEX_BUF(uint,  COMMON_SH_INDICES_OFFSETS,  68, PER_FRAME_SET);
DECLARE_TEX_2D(COMMON_SSR,                       69, PER_FRAME_SET, 1, PS_SMP_CLAMP_LINEAR, 0);
DECLARE_TEX_2D(COMMON_SSR_A,                     70, PER_FRAME_SET, 1, PS_SMP_CLAMP_LINEAR, 0);
DECLARE_TEX_2D(COMMON_WATER_REFRACTION,          71, PER_FRAME_SET, 1, PS_SMP_CLAMP_LINEAR, 0); 
DECLARE_TEX_3D_UINT(COMMON_DECAL_GRID,           72, PER_FRAME_SET, 1, 0                  , 0);
DECLARE_TEX_3D(COMMON_FOG_GRID,                  73, PER_FRAME_SET, 1, PS_SMP_CLAMP_LINEAR, 0);
DECLARE_TEX_2D(COMMON_SSGI,                      74, PER_FRAME_SET, 1, PS_SMP_CLAMP_LINEAR, 0);
// 75 - 76 free
DECLARE_TEX_2D(COMMON_DEPTH,                     77, PER_FRAME_SET, 1, PS_SMP_CLAMP_POINT,  0);  // fullscreen depth buffer
DECLARE_TEX_2D(COMMON_SHAPE_DATA,                78, PER_DRAW_TX_SET, 1, PS_SMP_CLAMP_POINT,  0);
DECLARE_TEX_2D(COMMON_SHAPE_ID,                  79, PER_DRAW_TX_SET, 1, PS_SMP_CLAMP_POINT,  0);
DECLARE_TEX_CUBE_A(COMMON_CUBE_ARR,              80, PER_FRAME_SET, 1, PS_SMP_CLAMP_LINEAR, 0);
DECLARE_TEX_2D_A(COMMON_FOG_DENSITY_INTEGRALS,   81, PER_FRAME_SET, 1, PS_SMP_CLAMP_LINEAR, 0);
//DECLARE_TEX_2D_UINT(COMMON_LIGHT_INDEX0,         82, 1, 0,                   0);
//DECLARE_TEX_2D_UINT(COMMON_LIGHT_INDEX1,         83, 1, 0,                   0);
DECLARE_TEX_2D(COMMON_TERRAIN_COLORIZE_MAP,      82, PER_DRAW_TX_SET, 1, PS_SMP_CLAMP_LINEAR,   0);
DECLARE_TEX_2D(COMMON_TERRAIN_WETNESS_MAP,       83, PER_DRAW_TX_SET, 1, PS_SMP_CLAMP_LINEAR,   0);

DECLARE_TEX_2D(COMMON_ESM_ATLAS,                 84, PER_FRAME_SET, 1, PS_SMP_CLAMP_LINEAR, 0);
// 85 is used for projective decals data
// These are set up in group!
DECLARE_TEX_3D_UINT(COMMON_LIGHT_VOLUME_BITMASK0,86, PER_FRAME_SET, 1, 0,                   0); // no_shadow, point
DECLARE_TEX_3D_UINT(COMMON_LIGHT_VOLUME_BITMASK1,87, PER_FRAME_SET, 1, 0,                   0); // no_shadow, spot
DECLARE_TEX_3D_UINT(COMMON_LIGHT_VOLUME_BITMASK2,88, PER_FRAME_SET, 1, 0,                   0); // shadow, point
DECLARE_TEX_3D_UINT(COMMON_LIGHT_VOLUME_BITMASK3,89, PER_FRAME_SET, 1, 0,                   0); // shadow, spot
DECLARE_TEX_3D_UINT(COMMON_LIGHT_VOLUME_BITMASK4,90, PER_FRAME_SET, 1, 0,                   0); // no_shadow and shadow, spot, proj_tex
DECLARE_TEX_3D_UINT(COMMON_TRANSP_SM_MASK,       91, PER_FRAME_SET, 1, 0,                   0);
DECLARE_TEX_3D_UINT(COMMON_CUBEMAP_VOLUME_MASK,  92, PER_FRAME_SET, 1, 0,                   0);
// End of group
DECLARE_TEX_2D_A(COMMON_LIGHT_TEX_ARR,           93, PER_FRAME_SET, 1, PS_SMP_BORDER_LINEAR, TFF_SRGB);
DECLARE_TEX_STRUCTURED_BUF(TPL_COMMON_DATA, TPL_COMMON_DATA_BUF, 94, PER_FRAME_SET);

DECLARE_TEX_2D(COMMON_WIND_ROOTS,                95, PER_FRAME_SET, 1, PS_SMP_WRAP_LINEAR,  0);
DECLARE_TEX_3D(COMMON_WIND_PIVOTS,               96, PER_FRAME_SET, 1, PS_SMP_WRAP_LINEAR,  0);

// 97-103 now is free

DECLARE_TEX_2D(COMMON_STATIC_SM,                104, PER_FRAME_SET, 1, PS_SMP_BORDER_CMP,   0);
DECLARE_TEX_2D(COMMON_STATIC_SM_QUARTER,        105, PER_FRAME_SET, 1, PS_SMP_BORDER_CMP,   0);
DECLARE_TEX_2D(COMMON_HM,                       106, PER_FRAME_SET, 1, PS_SMP_CLAMP_POINT,  0);
DECLARE_TEX_2D(COMMON_DECAL_ATLAS_DIFF,         107, PER_FRAME_SET, 1, PS_SMP_CLAMP_LINEAR, 0);
DECLARE_TEX_2D(COMMON_DECAL_ATLAS_NM,           108, PER_FRAME_SET, 1, PS_SMP_CLAMP_LINEAR, 0);
DECLARE_TEX_2D(COMMON_DECAL_ATLAS_SPEC,         109, PER_FRAME_SET, 1, PS_SMP_CLAMP_LINEAR, 0);
DECLARE_TEX_2D(COMMON_DECAL_ATLAS_HM,           110, PER_FRAME_SET, 1, PS_SMP_CLAMP_LINEAR, 0);
// 111 used for skeletal animation's data
DECLARE_TEX_2D(COMMON_DEPTH_HALF_MIN,           112, PER_FRAME_SET, 1, PS_SMP_CLAMP_POINT,  0);  // halfscreen depth buffer (dowmsampled with MIN)

DECLARE_TEX_BUF(uint2,  LWI_INST_INDEX_BUF,      113, PER_FRAME_SET); // instance indices
DECLARE_TEX_BUF(float4, LWI_INST_DATA_BUF,       114, PER_DRAW_TX_SET); // CB_PIX_OBJ data. matrices/quat/bones.
DECLARE_TEX_BUF(uint,   LWI_INST_DYN_DATA_BUF,   115, PER_FRAME_SET); // CB_PIX_OBJ dynamic per frame data. lod factors
DECLARE_TEX_BUF(uint ,  LWI_INST_DEBUG_DATA_BUF, 116, PER_FRAME_SET); // CB_PIX_OBJ debug data. inst UID for cursor picking.

DECLARE_TEX_2D_A(COMMON_SH_3D_ARRAY0,           117, PER_FRAME_SET, 1, PS_SMP_CLAMP_LINEAR, 0); // L0 rgb
DECLARE_TEX_2D_A(COMMON_SH_3D_ARRAY1,           118, PER_FRAME_SET, 1, PS_SMP_CLAMP_LINEAR, 0); // L1 rgb
DECLARE_TEX_2D_A(COMMON_SH_3D_ARRAY2,           119, PER_FRAME_SET, 1, PS_SMP_CLAMP_LINEAR, 0); // L1 rgb
DECLARE_TEX_2D_A(COMMON_SH_3D_ARRAY3,           120, PER_FRAME_SET, 1, PS_SMP_CLAMP_LINEAR, 0); // L1 rgb

DECLARE_TEX_2D_A (COMMON_TERRAIN_BAKED_ALBEDO,    122, PER_DRAW_TX_SET, 1, PS_SMP_CLAMP_ANISO, 0);
DECLARE_TEX_2D_A (COMMON_TERRAIN_BAKED_ROUGHNESS_METALNESS,    123, PER_DRAW_TX_SET, 1, PS_SMP_CLAMP_ANISO,  0);
DECLARE_TEX_2D_A (COMMON_TERRAIN_BAKED_NORMALS,      124, PER_DRAW_TX_SET, 1, PS_SMP_CLAMP_ANISO,  0);
DECLARE_TEX_2D_A (COMMON_TERRAIN_BAKED_DISPLACEMENT, 125, PER_DRAW_TX_SET, 1, PS_SMP_CLAMP_ANISO,  0);
DECLARE_TEX_2D   (COMMON_TERRAIN_HEIGHT,             126, PER_DRAW_TX_SET, 1, PS_SMP_CLAMP_LINEAR, 0);

// todo: check for bindless texturing support not the specific api
#if defined(_API_VULKAN) || defined(__cplusplus)
DECLARE_TEX_2D(COMMON_DECAL_TEXTURES,                                 127, PER_FRAME_SET, 256, PS_SMP_CLAMP_LINEAR, 0);
#endif

// ----------------------------------------------------------------------------
//
// Regular buffer binding
//
// ----------------------------------------------------------------------------

//
// Skin constant space
//
// Regular skinning 
#define VS_COMMON_SKIN_MATR_SIZE         3
#define VS_COMMON_SKIN_QUAT_SIZE         2
#define VS_COMMON_SKIN_DATA_SIZE         2

// Skin compound
#define MAX_VS_SKIN_COMP_OBJECTS              (MAX_VS_SKIN_DATA_CONSTS        / (VS_COMMON_SKIN_COMP_MATR_SIZE + VS_COMMON_SKIN_COMP_DATA_SIZE))
#define MAX_VS_SKIN_COMP_TRANSP_OBJECTS       (MAX_VS_SKIN_DATA_TRANSP_CONSTS / (VS_COMMON_SKIN_COMP_MATR_SIZE + VS_COMMON_SKIN_COMP_DATA_SIZE))
#define VS_COMMON_SKIN_COMP_OFFSET_TEX_OFFSET 0
#define VS_COMMON_SKIN_COMP_OFFSET_COLOR      1

DECLARE_REGULAR_BUFFER_USAGE(RB_SKIN_DATA, SKINDATA_BINDING_LOCATION, PER_DRAW_TX_SET, 65536, USAGE_VS);
      
// Decals data

#ifndef __cplusplus
// todo: use buffer<float4> or share structure type with decal shader
struct COMMON_DECAL_DATA {
   float3 center;
   int    matId;
   
   float3 halfSize;
   int    rendPrior;
   
   float3 vx;
   int    blendMode;
   
   float3 vy;
   float  alpha;
   
   float3 vz;
   int    gameMtl;
   
   int    objMask;
   float  zFade;
   int    _pad2;
   int    _pad3;
};
#else
struct COMMON_DECAL_DATA {
   //m4dV center;
   //m4dV halfSize;
   //m4dV vx, vy, vz;
   //
   //m4dVINT _pad;
};
#endif
DECLARE_TEX_STRUCTURED_BUF (COMMON_DECAL_DATA, COMMON_DECAL_VOLUMES, 85, PER_FRAME_SET);
      
// ----------------------------------------------------------------------------
//
// Common UAV binding
//
// ----------------------------------------------------------------------------

#define OBJ_ID_BUF_SIZE     100
#define OBJ_ID_DATA_SIZE_U1 5

#ifdef __cplusplus
struct COMMON_OBJ_ID_DATA {
   unsigned int instID; // to get debug cursor info
   unsigned int objId_splitID;
   unsigned int lwiUID[2]; // to send to SSL
   float        depth;
};

//CASSERT(OBJ_ID_DATA_SIZE_U1 * sizeof(UINT) == sizeof(COMMON_OBJ_ID_DATA));
#endif

DECLARE_UAV_TEX_2D_UINT(texObjID, TEXOBJID_BINDING_LOCATION, PER_FRAME_SET, 1);


// ----------------------------------------------------------------------------
//
// Samplers binding
//
// ----------------------------------------------------------------------------
DECLARE_SAMPLERS(PS_SAMPLERS, 0, PER_FRAME_SET, MAX_PS_SAMPLERS);
DECLARE_SAMPLERS_CMP(PS_SAMPLERS_CMP, MAX_PS_SAMPLERS, PER_FRAME_SET, MAX_PS_CMP_SAMPLERS);

END_REFLECTION_TABLE
   
#endif // PS_REGISTERS_FX_INCLUDED
