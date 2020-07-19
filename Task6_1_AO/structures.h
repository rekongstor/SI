#pragma once

struct siVertex
{
   XMFLOAT4 position;
   XMFLOAT4 normal;
   XMFLOAT2 uv;
};

struct mainConstBuff
{
   XMFLOAT4X4 vpMatrix;
   XMFLOAT4 camPos;
   XMFLOAT4 lightDirection;
   XMFLOAT4 lightColor;
   XMFLOAT4 ambientColor;
};

typedef uint8_t FfxCacaoBool;
static const FfxCacaoBool FFX_CACAO_TRUE = 1;
static const FfxCacaoBool FFX_CACAO_FALSE = 0;

/**
	The return codes for the API functions.
*/
typedef enum FfxCacaoStatus {
	FFX_CACAO_STATUS_OK = 0,
	FFX_CACAO_STATUS_INVALID_ARGUMENT = -1,
	FFX_CACAO_STATUS_INVALID_POINTER = -2,
	FFX_CACAO_STATUS_OUT_OF_MEMORY = -3,
	FFX_CACAO_STATUS_FAILED = -4,
} FfxCacaoStatus;

/**
	The quality levels that FidelityFX CACAO can generate SSAO at. This affects the number of samples taken for generating SSAO.
*/
typedef enum FfxCacaoQuality {
	FFX_CACAO_QUALITY_LOWEST = 0,
	FFX_CACAO_QUALITY_LOW = 1,
	FFX_CACAO_QUALITY_MEDIUM = 2,
	FFX_CACAO_QUALITY_HIGH = 3,
	FFX_CACAO_QUALITY_HIGHEST = 4,
} FfxCacaoQuality;

/**
	A structure representing a 4x4 matrix of floats. The matrix is stored in row major order in memory.
*/
typedef struct FfxCacaoMatrix4x4 {
	float elements[4][4];
} FfxCacaoMatrix4x4;

/**
	A structure for the settings used by FidelityFX CACAO. These settings may be updated with each draw call.
*/
typedef struct FfxCacaoSettings {
	float           radius;                            ///< [0.0,  ~ ] World (view) space size of the occlusion sphere.
	float           shadowMultiplier;                  ///< [0.0, 5.0] Effect strength linear multiplier
	float           shadowPower;                       ///< [0.5, 5.0] Effect strength pow modifier
	float           shadowClamp;                       ///< [0.0, 1.0] Effect max limit (applied after multiplier but before blur)
	float           horizonAngleThreshold;             ///< [0.0, 0.2] Limits self-shadowing (makes the sampling area less of a hemisphere, more of a spherical cone, to avoid self-shadowing and various artifacts due to low tessellation and depth buffer imprecision, etc.)
	float           fadeOutFrom;                       ///< [0.0,  ~ ] Distance to start start fading out the effect.
	float           fadeOutTo;                         ///< [0.0,  ~ ] Distance at which the effect is faded out.
	FfxCacaoQuality qualityLevel;                      ///<            Effect quality, affects number of taps etc
	float           adaptiveQualityLimit;              ///< [0.0, 1.0] (only for Quality Level 3)
	uint32_t        blurPassCount;                     ///< [  0,   8] Number of edge-sensitive smart blur passes to apply
	float           sharpness;                         ///< [0.0, 1.0] (How much to bleed over edges; 1: not at all, 0.5: half-half; 0.0: completely ignore edges)
	float           temporalSupersamplingAngleOffset;  ///< [0.0,  PI] Used to rotate sampling kernel; If using temporal AA / supersampling, suggested to rotate by ( (frame%3)/3.0*PI ) or similar. Kernel is already symmetrical, which is why we use PI and not 2*PI.
	float           temporalSupersamplingRadiusOffset; ///< [0.0, 2.0] Used to scale sampling kernel; If using temporal AA / supersampling, suggested to scale by ( 1.0f + (((frame%3)-1.0)/3.0)*0.1 ) or similar.
	float           detailShadowStrength;              ///< [0.0, 5.0] Used for high-res detail AO using neighboring depth pixels: adds a lot of detail but also reduces temporal stability (adds aliasing).
	FfxCacaoBool    generateNormals;                   ///< This option should be set to FFX_CACAO_TRUE if FidelityFX-CACAO should reconstruct a normal buffer from the depth buffer. It is required to be FFX_CACAO_TRUE if no normal buffer is provided.
	float           bilateralSigmaSquared;             ///< [0.0,  ~ ] Sigma squared value for use in bilateral upsampler giving Gaussian blur term. Should be greater than 0.0. 
	float           bilateralSimilarityDistanceSigma;  ///< [0.0,  ~ ] Sigma squared value for use in bilateral upsampler giving similarity weighting for neighbouring pixels. Should be greater than 0.0.
} FfxCacaoSettings;

static const FfxCacaoSettings FFX_CACAO_DEFAULT_SETTINGS = {
	/* radius                            */ 1.2f,
	/* shadowMultiplier                  */ 1.0f,
	/* shadowPower                       */ 1.50f,
	/* shadowClamp                       */ 0.98f,
	/* horizonAngleThreshold             */ 0.06f,
	/* fadeOutFrom                       */ 50.0f,
	/* fadeOutTo                         */ 300.0f,
	/* qualityLevel                      */ FFX_CACAO_QUALITY_HIGHEST,
	/* adaptiveQualityLimit              */ 0.45f,
	/* blurPassCount                     */ 2,
	/* sharpness                         */ 0.98f,
	/* temporalSupersamplingAngleOffset  */ 0.0f,
	/* temporalSupersamplingRadiusOffset */ 0.0f,
	/* detailShadowStrength              */ 0.5f,
	/* generateNormals                   */ FFX_CACAO_FALSE,
	/* bilateralSigmaSquared             */ 5.0f,
	/* bilateralSimilarityDistanceSigma  */ 0.01f,
};

typedef struct FfxCacaoConstants {
	float                   DepthUnpackConsts[2];
	float                   CameraTanHalfFOV[2];

	float                   NDCToViewMul[2];
	float                   NDCToViewAdd[2];

	float                   DepthBufferUVToViewMul[2];
	float                   DepthBufferUVToViewAdd[2];

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

	float                   PatternRotScaleMatrices[5][4];

	float                   NormalsUnpackMul;
	float                   NormalsUnpackAdd;
	float                   DetailAOStrength;
	float                   Dummy0;

	float                   SSAOBufferDimensions[2];
	float                   SSAOBufferInverseDimensions[2];

	float                   DepthBufferDimensions[2];
	float                   DepthBufferInverseDimensions[2];

	int                     DepthBufferOffset[2];
	float                   PerPassFullResUVOffset[2];

	float                   InputOutputBufferDimensions[2];
	float                   InputOutputBufferInverseDimensions[2];

	float                   ImportanceMapDimensions[2];
	float                   ImportanceMapInverseDimensions[2];

	float                   DeinterleavedDepthBufferDimensions[2];
	float                   DeinterleavedDepthBufferInverseDimensions[2];

	float                   DeinterleavedDepthBufferOffset[2];
	float                   DeinterleavedDepthBufferNormalisedOffset[2];

} FfxCacaoConstants;


typedef struct ScreenSizeInfo {
	uint32_t width;
	uint32_t height;
	uint32_t halfWidth;
	uint32_t halfHeight;
	uint32_t quarterWidth;
	uint32_t quarterHeight;
	uint32_t eighthWidth;
	uint32_t eighthHeight;
	uint32_t depthBufferWidth;
	uint32_t depthBufferHeight;
	uint32_t depthBufferHalfWidth;
	uint32_t depthBufferHalfHeight;
	uint32_t depthBufferQuarterWidth;
	uint32_t depthBufferQuarterHeight;
	uint32_t depthBufferOffsetX;
	uint32_t depthBufferOffsetY;
	uint32_t depthBufferHalfOffsetX;
	uint32_t depthBufferHalfOffsetY;
} ScreenSizeInfo;

typedef struct BufferSizeInfo {
	uint32_t inputOutputBufferWidth;
	uint32_t inputOutputBufferHeight;

	uint32_t ssaoBufferWidth;
	uint32_t ssaoBufferHeight;

	uint32_t depthBufferXOffset;
	uint32_t depthBufferYOffset;

	uint32_t depthBufferWidth;
	uint32_t depthBufferHeight;

	uint32_t deinterleavedDepthBufferXOffset;
	uint32_t deinterleavedDepthBufferYOffset;

	uint32_t deinterleavedDepthBufferWidth;
	uint32_t deinterleavedDepthBufferHeight;

	uint32_t importanceMapWidth;
	uint32_t importanceMapHeight;
};

static const FfxCacaoMatrix4x4 FFX_CACAO_IDENTITY_MATRIX = {
	{ { 1.0f, 0.0f, 0.0f, 0.0f },
	  { 0.0f, 1.0f, 0.0f, 0.0f },
	  { 0.0f, 0.0f, 1.0f, 0.0f },
	  { 0.0f, 0.0f, 0.0f, 1.0f } }
};

#define MATRIX_ROW_MAJOR_ORDER 1
#define MAX_BLUR_PASSES 8

#define FFX_CACAO_MIN(x, y) (((x) < (y)) ? (x) : (y))
#define FFX_CACAO_MAX(x, y) (((x) > (y)) ? (x) : (y))
#define FFX_CACAO_CLAMP(value, lower, upper) FFX_CACAO_MIN(FFX_CACAO_MAX(value, lower), upper)

struct csConstBuff
{
	XMFLOAT4X4 vpMatrixInv;
	XMFLOAT4X4 vMatrix;
	float width;
	float height;
	FfxCacaoConstants cacaoConst;
};
