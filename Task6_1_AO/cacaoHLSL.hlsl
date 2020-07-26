#pragma once

struct CACAOConstants
{
	float2                  DepthUnpackConsts;
	float2                  CameraTanHalfFOV;

	float2                  NDCToViewMul;
	float2                  NDCToViewAdd;

	float2                  DepthBufferUVToViewMul;
	float2                  DepthBufferUVToViewAdd;

	float                   EffectRadius;                           // world (viewspace) maximum size of the shadow
	float                   EffectShadowStrength;                   // global strength of the effect (0 - 5)
	float                   EffectShadowPow;
	float                   EffectShadowClamp;

	float                   EffectFadeOutMul;                       // effect fade out from distance (ex. 25)
	float                   EffectFadeOutAdd;                       // effect fade out to distance   (ex. 100)
	float                   EffectHorizonAngleThreshold;            // limit errors on slopes and caused by insufficient geometry tessellation (0.05 to 0.5)
	float                   EffectSamplingRadiusNearLimitRec;          // if viewspace pixel closer than this, don't enlarge shadow sampling radius anymore (makes no sense to grow beyond some distance, not enough samples to cover everything, so just limit the shadow growth; could be SSAOSettingsFadeOutFrom * 0.1 or less)

	float                   DepthPrecisionOffsetMod;
	float                   NegRecEffectRadius;                     // -1.0 / EffectRadius
	float                   LoadCounterAvgDiv;                      // 1.0 / ( halfDepthMip[SSAO_DEPTH_MIP_LEVELS-1].sizeX * halfDepthMip[SSAO_DEPTH_MIP_LEVELS-1].sizeY )
	float                   AdaptiveSampleCountLimit;

	float                   InvSharpness;
	int                     PassIndex;
	float                   BilateralSigmaSquared;
	float                   BilateralSimilarityDistanceSigma;

	float4                  PatternRotScaleMatrices[5];

	float                   NormalsUnpackMul;
	float                   NormalsUnpackAdd;
	float                   DetailAOStrength;
	float                   Dummy0;

	float2                  SSAOBufferDimensions;
	float2                  SSAOBufferInverseDimensions;

	float2                  DepthBufferDimensions;
	float2                  DepthBufferInverseDimensions;

	int2                    DepthBufferOffset;
	float2                  PerPassFullResUVOffset;

	float2                  OutputBufferDimensions;
	float2                  OutputBufferInverseDimensions;

	float2                  ImportanceMapDimensions;
	float2                  ImportanceMapInverseDimensions;

	float2                  DeinterleavedDepthBufferDimensions;
	float2                  DeinterleavedDepthBufferInverseDimensions;

	float2                  DeinterleavedDepthBufferOffset;
	float2                  DeinterleavedDepthBufferNormalisedOffset;

#if SSAO_ENABLE_NORMAL_WORLD_TO_VIEW_CONVERSION
	float4x4                NormalsWorldToViewspaceMatrix;
#endif
};

static const float4 g_samplePatternMain[] =
{
	 0.78488064,  0.56661671,  1.500000, -0.126083,     0.26022232, -0.29575172,  1.500000, -1.064030,     0.10459357,  0.08372527,  1.110000, -2.730563,    -0.68286800,  0.04963045,  1.090000, -0.498827,
	-0.13570161, -0.64190155,  1.250000, -0.532765,    -0.26193795, -0.08205118,  0.670000, -1.783245,    -0.61177456,  0.66664219,  0.710000, -0.044234,     0.43675563,  0.25119025,  0.610000, -1.167283,
	 0.07884444,  0.86618668,  0.640000, -0.459002,    -0.12790935, -0.29869005,  0.600000, -1.729424,    -0.04031125,  0.02413622,  0.600000, -4.792042,     0.16201244, -0.52851415,  0.790000, -1.067055,
	-0.70991218,  0.47301072,  0.640000, -0.335236,     0.03277707, -0.22349690,  0.600000, -1.982384,     0.68921727,  0.36800742,  0.630000, -0.266718,     0.29251814,  0.37775412,  0.610000, -1.422520,
	-0.12224089,  0.96582592,  0.600000, -0.426142,     0.11071457, -0.16131058,  0.600000, -2.165947,     0.46562141, -0.59747696,  0.600000, -0.189760,    -0.51548797,  0.11804193,  0.600000, -1.246800,
	 0.89141309, -0.42090443,  0.600000,  0.028192,    -0.32402530, -0.01591529,  0.600000, -1.543018,     0.60771245,  0.41635221,  0.600000, -0.605411,     0.02379565, -0.08239821,  0.600000, -3.809046,
	 0.48951152, -0.23657045,  0.600000, -1.189011,    -0.17611565, -0.81696892,  0.600000, -0.513724,    -0.33930185, -0.20732205,  0.600000, -1.698047,    -0.91974425,  0.05403209,  0.600000,  0.062246,
	-0.15064627, -0.14949332,  0.600000, -1.896062,     0.53180975, -0.35210401,  0.600000, -0.758838,     0.41487166,  0.81442589,  0.600000, -0.505648,    -0.24106961, -0.32721516,  0.600000, -1.665244
};

#define SSAO_MAX_TAPS (32)
#define SSAO_MAX_REF_TAPS (512)
#define SSAO_ADAPTIVE_TAP_BASE_COUNT (5)
#define SSAO_ADAPTIVE_TAP_FLEXIBLE_COUNT (SSAO_MAX_TAPS - SSAO_ADAPTIVE_TAP_BASE_COUNT)
#define SSAO_DEPTH_MIP_LEVELS (4)

// these values can be changed (up to SSAO_MAX_TAPS) with no changes required elsewhere; values for 4th and 5th preset are ignored but array needed to avoid compilation errors
// the actual number of texture samples is two times this value (each "tap" has two symmetrical depth texture samples)
static const uint g_numTaps[5] = { 3, 5, 12, 0, 0 };


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Optional parts that can be enabled for a required quality preset level and above (0 == Low, 1 == Medium, 2 == High, 3 == Highest/Adaptive, 4 == reference/unused )
// Each has its own cost. To disable just set to 5 or above.
//
// (experimental) tilts the disk (although only half of the samples!) towards surface normal; this helps with effect uniformity between objects but reduces effect distance and has other side-effects
#define SSAO_TILT_SAMPLES_ENABLE_AT_QUALITY_PRESET                      (99)        // to disable simply set to 99 or similar
#define SSAO_TILT_SAMPLES_AMOUNT                                        (0.4)
//
#define SSAO_HALOING_REDUCTION_ENABLE_AT_QUALITY_PRESET                 (1)         // to disable simply set to 99 or similar
#define SSAO_HALOING_REDUCTION_AMOUNT                                   (0.6)       // values from 0.0 - 1.0, 1.0 means max weighting (will cause artifacts, 0.8 is more reasonable)
//
#define SSAO_NORMAL_BASED_EDGES_ENABLE_AT_QUALITY_PRESET                (2) //2        // to disable simply set to 99 or similar
#define SSAO_NORMAL_BASED_EDGES_DOT_THRESHOLD                           (0.5)       // use 0-0.1 for super-sharp normal-based edges
//
#define SSAO_DETAIL_AO_ENABLE_AT_QUALITY_PRESET                         (1) //1         // whether to use DetailAOStrength; to disable simply set to 99 or similar
//
#define SSAO_DEPTH_MIPS_ENABLE_AT_QUALITY_PRESET                        (2)         // !!warning!! the MIP generation on the C++ side will be enabled on quality preset 2 regardless of this value, so if changing here, change the C++ side too
#define SSAO_DEPTH_MIPS_GLOBAL_OFFSET                                   (-4.3)      // best noise/quality/performance tradeoff, found empirically
//
// !!warning!! the edge handling is hard-coded to 'disabled' on quality level 0, and enabled above, on the C++ side; while toggling it here will work for 
// testing purposes, it will not yield performance gains (or correct results)
#define SSAO_DEPTH_BASED_EDGES_ENABLE_AT_QUALITY_PRESET                 (1)     
//
#define SSAO_REDUCE_RADIUS_NEAR_SCREEN_BORDER_ENABLE_AT_QUALITY_PRESET  (99)        // 99 means disabled; only helpful if artifacts at the edges caused by lack of out of screen depth data are not acceptable with the depth sampler in either clamp or mirror modes
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


SamplerState              g_PointClampSampler : register(s0);   // corresponds to SSAO_SAMPLERS_SLOT0
SamplerState              g_PointMirrorSampler : register(s1);   // corresponds to SSAO_SAMPLERS_SLOT2
SamplerState              g_LinearClampSampler : register(s2);   // corresponds to SSAO_SAMPLERS_SLOT1
SamplerState              g_ViewspaceDepthTapSampler : register(s3);   // corresponds to SSAO_SAMPLERS_SLOT3
SamplerState              g_ZeroTextureSampler : register(s4);

cbuffer SSAOConstantsBuffer : register(b0)    // corresponds to SSAO_CONSTANTS_BUFFERSLOT
{
	CACAOConstants        g_CACAOConsts;
}