SamplerState g_PointClampSampler : register(s0); // corresponds to SSAO_SAMPLERS_SLOT0
SamplerState g_PointMirrorSampler : register(s1); // corresponds to SSAO_SAMPLERS_SLOT2
SamplerState g_LinearClampSampler : register(s2); // corresponds to SSAO_SAMPLERS_SLOT1
SamplerState g_ViewspaceDepthTapSampler : register(s3); // corresponds to SSAO_SAMPLERS_SLOT3
SamplerState g_ZeroTextureSampler : register(s4);

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

#define INTELSSAO_MAIN_DISK_SAMPLE_COUNT (32)

#define SSAO_MAX_TAPS (32)
#define SSAO_MAX_REF_TAPS (512)
#define SSAO_ADAPTIVE_TAP_BASE_COUNT (5)
#define SSAO_ADAPTIVE_TAP_FLEXIBLE_COUNT (SSAO_MAX_TAPS - SSAO_ADAPTIVE_TAP_BASE_COUNT)
#define SSAO_DEPTH_MIP_LEVELS (4)

struct PrepareNormalsInputDepths
{
   float depth_10;
   float depth_20;

   float depth_01;
   float depth_11;
   float depth_21;
   float depth_31;

   float depth_02;
   float depth_12;
   float depth_22;
   float depth_32;

   float depth_13;
   float depth_23;
};

struct CACAOConstants
{
   float2 DepthUnpackConsts;
   float2 CameraTanHalfFOV;

   float2 NDCToViewMul;
   float2 NDCToViewAdd;

   float2 DepthBufferUVToViewMul;
   float2 DepthBufferUVToViewAdd;

   float EffectRadius; // world (viewspace) maximum size of the shadow
   float EffectShadowStrength; // global strength of the effect (0 - 5)
   float EffectShadowPow;
   float EffectShadowClamp;

   float EffectFadeOutMul; // effect fade out from distance (ex. 25)
   float EffectFadeOutAdd; // effect fade out to distance   (ex. 100)
   float EffectHorizonAngleThreshold;
   // limit errors on slopes and caused by insufficient geometry tessellation (0.05 to 0.5)
   float EffectSamplingRadiusNearLimitRec;
   // if viewspace pixel closer than this, don't enlarge shadow sampling radius anymore (makes no sense to grow beyond some distance, not enough samples to cover everything, so just limit the shadow growth; could be SSAOSettingsFadeOutFrom * 0.1 or less)

   float DepthPrecisionOffsetMod;
   float NegRecEffectRadius; // -1.0 / EffectRadius
   float LoadCounterAvgDiv;
   // 1.0 / ( halfDepthMip[SSAO_DEPTH_MIP_LEVELS-1].sizeX * halfDepthMip[SSAO_DEPTH_MIP_LEVELS-1].sizeY )
   float AdaptiveSampleCountLimit;

   float InvSharpness;
   int PassIndex;
   float BilateralSigmaSquared;
   float BilateralSimilarityDistanceSigma;

   float4 PatternRotScaleMatrices[5];

   float NormalsUnpackMul;
   float NormalsUnpackAdd;
   float DetailAOStrength;
   float Dummy0;

   float2 SSAOBufferDimensions;
   float2 SSAOBufferInverseDimensions;

   float2 DepthBufferDimensions;
   float2 DepthBufferInverseDimensions;

   int2 DepthBufferOffset;
   float2 PerPassFullResUVOffset;

   float2 OutputBufferDimensions;
   float2 OutputBufferInverseDimensions;

   float2 ImportanceMapDimensions;
   float2 ImportanceMapInverseDimensions;

   float2 DeinterleavedDepthBufferDimensions;
   float2 DeinterleavedDepthBufferInverseDimensions;

   float2 DeinterleavedDepthBufferOffset;
   float2 DeinterleavedDepthBufferNormalisedOffset;
};

static const float4 g_samplePatternMain[INTELSSAO_MAIN_DISK_SAMPLE_COUNT] =
{
   0.78488064, 0.56661671, 1.500000, -0.126083, 0.26022232, -0.29575172, 1.500000, -1.064030, 0.10459357, 0.08372527,
   1.110000, -2.730563, -0.68286800, 0.04963045, 1.090000, -0.498827,
   -0.13570161, -0.64190155, 1.250000, -0.532765, -0.26193795, -0.08205118, 0.670000, -1.783245, -0.61177456,
   0.66664219, 0.710000, -0.044234, 0.43675563, 0.25119025, 0.610000, -1.167283,
   0.07884444, 0.86618668, 0.640000, -0.459002, -0.12790935, -0.29869005, 0.600000, -1.729424, -0.04031125, 0.02413622,
   0.600000, -4.792042, 0.16201244, -0.52851415, 0.790000, -1.067055,
   -0.70991218, 0.47301072, 0.640000, -0.335236, 0.03277707, -0.22349690, 0.600000, -1.982384, 0.68921727, 0.36800742,
   0.630000, -0.266718, 0.29251814, 0.37775412, 0.610000, -1.422520,
   -0.12224089, 0.96582592, 0.600000, -0.426142, 0.11071457, -0.16131058, 0.600000, -2.165947, 0.46562141, -0.59747696,
   0.600000, -0.189760, -0.51548797, 0.11804193, 0.600000, -1.246800,
   0.89141309, -0.42090443, 0.600000, 0.028192, -0.32402530, -0.01591529, 0.600000, -1.543018, 0.60771245, 0.41635221,
   0.600000, -0.605411, 0.02379565, -0.08239821, 0.600000, -3.809046,
   0.48951152, -0.23657045, 0.600000, -1.189011, -0.17611565, -0.81696892, 0.600000, -0.513724, -0.33930185,
   -0.20732205, 0.600000, -1.698047, -0.91974425, 0.05403209, 0.600000, 0.062246,
   -0.15064627, -0.14949332, 0.600000, -1.896062, 0.53180975, -0.35210401, 0.600000, -0.758838, 0.41487166, 0.81442589,
   0.600000, -0.505648, -0.24106961, -0.32721516, 0.600000, -1.665244
};
static const uint g_numTaps[5] = {3, 5, 12, 0, 0};

cbuffer cbPass : register(b0)
{
float4x4 vpMatrixInv;
float4x4 vMatrix;
float width;
float height;
CACAOConstants g_CACAOConsts;
}

RWTexture2D<float4> output: register(u0);
Texture2D input : register(t0);

float3 NDCToViewspace(float2 pos, float viewspaceDepth)
{
   float3 ret;

   ret.xy = (g_CACAOConsts.NDCToViewMul * pos.xy + g_CACAOConsts.NDCToViewAdd) * viewspaceDepth;

   ret.z = viewspaceDepth;

   return ret;
}

float CalculatePixelObscurance(float3 pixelNormal, float3 hitDelta, float falloffCalcMulSq)
{
   float lengthSq = dot(hitDelta, hitDelta);
   float NdotD = dot(pixelNormal, hitDelta) / sqrt(lengthSq);

   float falloffMult = max(0.0, lengthSq * falloffCalcMulSq + 1.0);

   return max(0, NdotD - g_CACAOConsts.EffectHorizonAngleThreshold) * falloffMult;
}

float3 DepthBufferUVToViewspace(float2 pos, float viewspaceDepth)
{
   float3 ret;
   ret.xy = (g_CACAOConsts.DepthBufferUVToViewMul * pos.xy + g_CACAOConsts.DepthBufferUVToViewAdd) * viewspaceDepth;
   ret.z = viewspaceDepth;
   return ret;
}


float3 CalculateNormal(const float4 edgesLRTB, float3 pixCenterPos, float3 pixLPos, float3 pixRPos, float3 pixTPos,
                       float3 pixBPos)
{
   // Get this pixel's viewspace normal
   float4 acceptedNormals = float4(edgesLRTB.x * edgesLRTB.z, edgesLRTB.z * edgesLRTB.y, edgesLRTB.y * edgesLRTB.w,
                                   edgesLRTB.w * edgesLRTB.x);

   pixLPos = normalize(pixLPos - pixCenterPos);
   pixRPos = normalize(pixRPos - pixCenterPos);
   pixTPos = normalize(pixTPos - pixCenterPos);
   pixBPos = normalize(pixBPos - pixCenterPos);

   float3 pixelNormal = float3(0, 0, -0.0005);
   pixelNormal += (acceptedNormals.x) * cross(pixLPos, pixTPos);
   pixelNormal += (acceptedNormals.y) * cross(pixTPos, pixRPos);
   pixelNormal += (acceptedNormals.z) * cross(pixRPos, pixBPos);
   pixelNormal += (acceptedNormals.w) * cross(pixBPos, pixLPos);
   pixelNormal = normalize(pixelNormal);

   return normalize(pixelNormal);
}

float4 CalculateEdges(const float centerZ, const float leftZ, const float rightZ, const float topZ, const float bottomZ)
{
   // slope-sensitive depth-based edge detection
   float4 edgesLRTB = float4(leftZ, rightZ, topZ, bottomZ) - centerZ;
   float4 edgesLRTBSlopeAdjusted = edgesLRTB + edgesLRTB.yxwz;
   edgesLRTB = min(abs(edgesLRTB), abs(edgesLRTBSlopeAdjusted));
   return saturate((1.3 - edgesLRTB / (centerZ * 0.040)));

   // cheaper version but has artifacts
   // edgesLRTB = abs( float4( leftZ, rightZ, topZ, bottomZ ) - centerZ; );
   // return saturate( ( 1.3 - edgesLRTB / (pixZ * 0.06 + 0.1) ) );
}


float3 CalcNormal(float2 uv, float2 pixelSize, int2 normalCoord)
{
   float3 p_10 = NDCToViewspace(uv + float2(+0.0f, -1.0f) * pixelSize,
                                input.GatherRed(g_PointClampSampler, uv, int2(+0, -1)));
   float3 p_01 = NDCToViewspace(uv + float2(-1.0f, +0.0f) * pixelSize,
                                input.GatherRed(g_PointClampSampler, uv, int2(-1, +0)));
   float3 p_11 = NDCToViewspace(uv + float2(+0.0f, +0.0f) * pixelSize,
                                input.GatherRed(g_PointClampSampler, uv, int2(+0, +0)));
   float3 p_21 = NDCToViewspace(uv + float2(+1.0f, +0.0f) * pixelSize,
                                input.GatherRed(g_PointClampSampler, uv, int2(+1, +0)));
   float3 p_12 = NDCToViewspace(uv + float2(+0.0f, +1.0f) * pixelSize,
                                input.GatherRed(g_PointClampSampler, uv, int2(+0, +1)));

   float4 edges_11 = CalculateEdges(p_11.z, p_01.z, p_21.z, p_10.z, p_12.z);

   float3 norm_11 = CalculateNormal(edges_11, p_11, p_01, p_21, p_10, p_12);

   return norm_11;
}

// calculate effect radius and fit our screen sampling pattern inside it
void CalculateRadiusParameters(const float pixCenterLength, const float2 pixelDirRBViewspaceSizeAtCenterZ,
                               out float pixLookupRadiusMod, out float effectRadius, out float falloffCalcMulSq)
{
   effectRadius = g_CACAOConsts.EffectRadius;

   // leaving this out for performance reasons: use something similar if radius needs to scale based on distance
   //effectRadius *= pow( pixCenterLength, g_CACAOConsts.RadiusDistanceScalingFunctionPow);

   // when too close, on-screen sampling disk will grow beyond screen size; limit this to avoid closeup temporal artifacts
   const float tooCloseLimitMod = saturate(pixCenterLength * g_CACAOConsts.EffectSamplingRadiusNearLimitRec) * 0.8 +
      0.2;

   effectRadius *= tooCloseLimitMod;

   // 0.85 is to reduce the radius to allow for more samples on a slope to still stay within influence
   pixLookupRadiusMod = (0.85 * effectRadius) / pixelDirRBViewspaceSizeAtCenterZ.x;

   // used to calculate falloff (both for AO samples and per-sample weights)
   falloffCalcMulSq = -1.0f / (effectRadius * effectRadius);
}

void SSAOTapInner(const int qualityLevel, inout float obscuranceSum, inout float weightSum, const float2 samplingUV,
                  const float mipLevel, const float3 pixCenterPos, const float3 negViewspaceDir, float3 pixelNormal,
                  const float falloffCalcMulSq, const float weightMod, const int dbgTapIndex)
{
   // get depth at sample
   float viewspaceSampleZ = input.SampleLevel(g_ViewspaceDepthTapSampler, samplingUV.xy, mipLevel).x;
   // * g_CACAOConsts.MaxViewspaceDepth;

   // convert to viewspace
   // float3 hitPos = NDCToViewspace(samplingUV.xy, viewspaceSampleZ).xyz;
   float3 hitPos = DepthBufferUVToViewspace(samplingUV.xy, viewspaceSampleZ).xyz;
   float3 hitDelta = hitPos - pixCenterPos;

   float obscurance = CalculatePixelObscurance(pixelNormal, hitDelta, falloffCalcMulSq);
   float weight = 1.0;

   if (qualityLevel >= SSAO_HALOING_REDUCTION_ENABLE_AT_QUALITY_PRESET)
   {
      //float reduct = max( 0, dot( hitDelta, negViewspaceDir ) );
      float reduct = max(0, -hitDelta.z); // cheaper, less correct version
      reduct = saturate(reduct * g_CACAOConsts.NegRecEffectRadius + 2.0);
      // saturate( 2.0 - reduct / g_CACAOConsts.EffectRadius );
      weight = SSAO_HALOING_REDUCTION_AMOUNT * reduct + (1.0 - SSAO_HALOING_REDUCTION_AMOUNT);
   }
   weight *= weightMod;
   obscuranceSum += obscurance * weight;
   weightSum += weight;
}

void SSAOTap(const int qualityLevel, inout float obscuranceSum, inout float weightSum, const int tapIndex,
             const float2x2 rotScale, const float3 pixCenterPos, const float3 negViewspaceDir, float3 pixelNormal,
             const float2 normalizedScreenPos, const float2 depthBufferUV, const float mipOffset,
             const float falloffCalcMulSq, float weightMod, float2 normXY, float normXYLength)
{
   float2 sampleOffset;
   float samplePow2Len;

   // patterns
   {
      float4 newSample = g_samplePatternMain[tapIndex];
      sampleOffset = mul(rotScale, newSample.xy);
      samplePow2Len = newSample.w; // precalculated, same as: samplePow2Len = log2( length( newSample.xy ) );
      weightMod *= newSample.z;
   }

   // snap to pixel center (more correct obscurance math, avoids artifacts)
   sampleOffset = round(sampleOffset);

   // calculate MIP based on the sample distance from the centre, similar to as described 
   // in http://graphics.cs.williams.edu/papers/SAOHPG12/.
   float mipLevel = (qualityLevel < SSAO_DEPTH_MIPS_ENABLE_AT_QUALITY_PRESET) ? (0) : (samplePow2Len + mipOffset);


   float2 samplingUV = sampleOffset * g_CACAOConsts.DeinterleavedDepthBufferInverseDimensions + depthBufferUV;

   SSAOTapInner(qualityLevel, obscuranceSum, weightSum, samplingUV, mipLevel, pixCenterPos, negViewspaceDir,
                pixelNormal, falloffCalcMulSq, weightMod, tapIndex * 2);

   // for the second tap, just use the mirrored offset
   float2 sampleOffsetMirroredUV = -sampleOffset;

   // tilt the second set of samples so that the disk is effectively rotated by the normal
   // effective at removing one set of artifacts, but too expensive for lower quality settings
   if (qualityLevel >= SSAO_TILT_SAMPLES_ENABLE_AT_QUALITY_PRESET)
   {
      float dotNorm = dot(sampleOffsetMirroredUV, normXY);
      sampleOffsetMirroredUV -= dotNorm * normXYLength * normXY;
      sampleOffsetMirroredUV = round(sampleOffsetMirroredUV);
   }

   // snap to pixel center (more correct obscurance math, avoids artifacts)
   float2 samplingMirroredUV = sampleOffsetMirroredUV * g_CACAOConsts.DeinterleavedDepthBufferInverseDimensions +
      depthBufferUV;

   SSAOTapInner(qualityLevel, obscuranceSum, weightSum, samplingMirroredUV, mipLevel, pixCenterPos, negViewspaceDir,
                pixelNormal, falloffCalcMulSq, weightMod, tapIndex * 2 + 1);
}

float ScreenSpaceToViewSpaceDepth(float screenDepth)
{
   float depthLinearizeMul = g_CACAOConsts.DepthUnpackConsts.x;
   float depthLinearizeAdd = g_CACAOConsts.DepthUnpackConsts.y;

   // Optimised version of "-cameraClipNear / (cameraClipFar - projDepth * (cameraClipFar - cameraClipNear)) * cameraClipFar"

   // Set your depthLinearizeMul and depthLinearizeAdd to:
   // depthLinearizeMul = ( cameraClipFar * cameraClipNear) / ( cameraClipFar - cameraClipNear );
   // depthLinearizeAdd = cameraClipFar / ( cameraClipFar - cameraClipNear );

   return depthLinearizeMul / (depthLinearizeAdd - screenDepth);
}

void GenerateSSAOShadowsInternal(out float outShadowTerm, out float4 outEdges, out float outWeight,
                                 const float2 SVPos/*, const float2 normalizedScreenPos*/, uniform int qualityLevel,
                                 bool adaptiveBase)
{
   float2 SVPosRounded = trunc(SVPos);
   uint2 SVPosui = uint2(SVPosRounded); //same as uint2( SVPos )

   const int numberOfTaps = (adaptiveBase) ? (SSAO_ADAPTIVE_TAP_BASE_COUNT) : (g_numTaps[qualityLevel]);
   float pixZ, pixLZ, pixTZ, pixRZ, pixBZ;

   float2 depthBufferUV = SVPos / float2(width, height);
   //(SVPos + 0.5f) * g_CACAOConsts.DeinterleavedDepthBufferInverseDimensions + g_CACAOConsts.DeinterleavedDepthBufferNormalisedOffset;
   float4 valuesUL = input.GatherRed(g_PointClampSampler, depthBufferUV, int2(-1, -1));
   float4 valuesBR = input.GatherRed(g_PointClampSampler, depthBufferUV);

   // get this pixel's viewspace depth
   pixZ = valuesUL.y;
   //float pixZ = g_ViewspaceDepthSource.SampleLevel( g_PointMirrorSampler, normalizedScreenPos, 0.0 ).x; // * g_CACAOConsts.MaxViewspaceDepth;

   // get left right top bottom neighbouring pixels for edge detection (gets compiled out on qualityLevel == 0)
   pixLZ = valuesUL.x;
   pixTZ = valuesUL.z;
   pixRZ = valuesBR.z;
   pixBZ = valuesBR.x;

   // float2 normalizedScreenPos = SVPosRounded * g_CACAOConsts.Viewport2xPixelSize + g_CACAOConsts.Viewport2xPixelSize_x_025;
   float2 normalizedScreenPos = SVPos / float2(width, height);
   float3 pixCenterPos = NDCToViewspace(normalizedScreenPos, pixZ); // g

   // Load this pixel's viewspace normal
   // uint2 fullResCoord = 2 * (SVPosui * 2 + g_CACAOConsts.PerPassFullResCoordOffset.xy);
   // float3 pixelNormal = LoadNormal(fullResCoord);
   int3 normalCoord = int3(SVPosui, g_CACAOConsts.PassIndex);

   float3 pixelNormal = CalcNormal(normalizedScreenPos, float2(1.f / width, 1.f / height), normalCoord);

   output[normalCoord.xy] = abs(float4(pixelNormal, 1));
   // optimized approximation of:  float2 pixelDirRBViewspaceSizeAtCenterZ = NDCToViewspace( normalizedScreenPos.xy + g_CACAOConsts._ViewportPixelSize.xy, pixCenterPos.z ).xy - pixCenterPos.xy;
   // const float2 pixelDirRBViewspaceSizeAtCenterZ = pixCenterPos.z * g_CACAOConsts.NDCToViewMul * g_CACAOConsts.Viewport2xPixelSize;
   const float2 pixelDirRBViewspaceSizeAtCenterZ = pixCenterPos.z * g_CACAOConsts.NDCToViewMul * g_CACAOConsts.
      SSAOBufferInverseDimensions;

   float pixLookupRadiusMod;
   float falloffCalcMulSq;

   // calculate effect radius and fit our screen sampling pattern inside it
   float effectViewspaceRadius;
   CalculateRadiusParameters(length(pixCenterPos), pixelDirRBViewspaceSizeAtCenterZ, pixLookupRadiusMod,
                             effectViewspaceRadius, falloffCalcMulSq);

   // calculate samples rotation/scaling
   float2x2 rotScale;
   {
      // reduce effect radius near the screen edges slightly; ideally, one would render a larger depth buffer (5% on each side) instead
      if (!adaptiveBase && (qualityLevel >= SSAO_REDUCE_RADIUS_NEAR_SCREEN_BORDER_ENABLE_AT_QUALITY_PRESET))
      {
         float nearScreenBorder = min(min(depthBufferUV.x, 1.0 - depthBufferUV.x),
                                      min(depthBufferUV.y, 1.0 - depthBufferUV.y));
         nearScreenBorder = saturate(10.0 * nearScreenBorder + 0.6);
         pixLookupRadiusMod *= nearScreenBorder;
      }

      // load & update pseudo-random rotation matrix
      uint pseudoRandomIndex = uint(SVPosRounded.y * 2 + SVPosRounded.x) % 5;
      float4 rs = g_CACAOConsts.PatternRotScaleMatrices[pseudoRandomIndex];
      rotScale = float2x2(rs.x * pixLookupRadiusMod, rs.y * pixLookupRadiusMod, rs.z * pixLookupRadiusMod,
                          rs.w * pixLookupRadiusMod);
   }

   // the main obscurance & sample weight storage
   float obscuranceSum = 0.0;
   float weightSum = 0.0;

   // edge mask for between this and left/right/top/bottom neighbour pixels - not used in quality level 0 so initialize to "no edge" (1 is no edge, 0 is edge)
   float4 edgesLRTB = float4(1.0, 1.0, 1.0, 1.0);

   // Move center pixel slightly towards camera to avoid imprecision artifacts due to using of 16bit depth buffer; a lot smaller offsets needed when using 32bit floats
   pixCenterPos *= g_CACAOConsts.DepthPrecisionOffsetMod;

   if (!adaptiveBase && (qualityLevel >= SSAO_DEPTH_BASED_EDGES_ENABLE_AT_QUALITY_PRESET))
   {
      edgesLRTB = CalculateEdges(pixZ, pixLZ, pixRZ, pixTZ, pixBZ);
   }


   // Used to tilt the second set of samples so that the disk is effectively rotated by the normal
   // effective at removing one set of artifacts, but too expensive for lower quality settings
   float2 normXY = float2(pixelNormal.x, pixelNormal.y);
   float normXYLength = length(normXY);
   normXY /= float2(normXYLength, -normXYLength);
   normXYLength *= SSAO_TILT_SAMPLES_AMOUNT;

   const float3 negViewspaceDir = -normalize(pixCenterPos);

   // standard, non-adaptive approach

   //SSAOHits prevHits = SSAOGetHits(qualityLevel, normalizedScreenPos, 0, mipOffset);


   [unroll]
   for (int i = 0; i < 1/*numberOfTaps*/; i++)
   {
      SSAOTap(qualityLevel, obscuranceSum, weightSum, i, rotScale, pixCenterPos, negViewspaceDir, pixelNormal,
              normalizedScreenPos, depthBufferUV, 0, falloffCalcMulSq, 1.0, normXY, normXYLength);
      // SSAOHits hits = SSAOGetHits(qualityLevel, normalizedScreenPos, i, mipOffset, rotScale);
      // SSAOAddHits(qualityLevel, pixCenterPos, pixelNormal, 1.0f, falloffCalcMulSq, weightSum, obscuranceSum, hits);
   }


   // early out for adaptive base - just output weight (used for the next pass)
   if (adaptiveBase)
   {
      float obscurance = obscuranceSum / weightSum;

      outShadowTerm = obscurance;
      outEdges = 0;
      outWeight = weightSum;
      return;
   }

   // calculate weighted average
   float obscurance = obscuranceSum / weightSum;

   // calculate fadeout (1 close, gradient, 0 far)
   float fadeOut = saturate(pixCenterPos.z * g_CACAOConsts.EffectFadeOutMul + g_CACAOConsts.EffectFadeOutAdd);

   // Reduce the SSAO shadowing if we're on the edge to remove artifacts on edges (we don't care for the lower quality one)
   if (!adaptiveBase && (qualityLevel >= SSAO_DEPTH_BASED_EDGES_ENABLE_AT_QUALITY_PRESET))
   {
      // float edgeCount = dot( 1.0-edgesLRTB, float4( 1.0, 1.0, 1.0, 1.0 ) );

      // when there's more than 2 opposite edges, start fading out the occlusion to reduce aliasing artifacts
      float edgeFadeoutFactor = saturate((1.0 - edgesLRTB.x - edgesLRTB.y) * 0.35) + saturate(
         (1.0 - edgesLRTB.z - edgesLRTB.w) * 0.35);

      // (experimental) if you want to reduce the effect next to any edge
      // edgeFadeoutFactor += 0.1 * saturate( dot( 1 - edgesLRTB, float4( 1, 1, 1, 1 ) ) );

      fadeOut *= saturate(1.0 - edgeFadeoutFactor);
   }

   // same as a bove, but a lot more conservative version
   // fadeOut *= saturate( dot( edgesLRTB, float4( 0.9, 0.9, 0.9, 0.9 ) ) - 2.6 );

   // strength
   obscurance = g_CACAOConsts.EffectShadowStrength * obscurance;

   // clamp
   obscurance = min(obscurance, g_CACAOConsts.EffectShadowClamp);

   // fadeout
   obscurance *= fadeOut;

   // conceptually switch to occlusion with the meaning being visibility (grows with visibility, occlusion == 1 implies full visibility), 
   // to be in line with what is more commonly used.
   float occlusion = 1.0 - obscurance;

   // modify the gradient
   // note: this cannot be moved to a later pass because of loss of precision after storing in the render target
   occlusion = pow(saturate(occlusion), g_CACAOConsts.EffectShadowPow);

   // outputs!
   outShadowTerm = occlusion; // Our final 'occlusion' term (0 means fully occluded, 1 means fully lit)
   outEdges = edgesLRTB;
   // These are used to prevent blurring across edges, 1 means no edge, 0 means edge, 0.5 means half way there, etc.
   outWeight = weightSum;
}

float PackEdges(float4 edgesLRTB)
{
   edgesLRTB = round(saturate(edgesLRTB) * 3.05);
   return dot(edgesLRTB, float4(64.0 / 255.0, 16.0 / 255.0, 4.0 / 255.0, 1.0 / 255.0));
}

void CSGenerateQ0(uint2 coord : SV_DispatchThreadID)
{
   float2 inPos = (float2)coord;
   float outShadowTerm;
   float outWeight;
   float4 outEdges;
   GenerateSSAOShadowsInternal(outShadowTerm, outEdges, outWeight, inPos.xy, 0, false);
   float2 out0;
   out0.x = outShadowTerm;
   out0.y = PackEdges(float4(1, 1, 1, 1)); // no edges in low quality
   //g_SSAOOutput[coord] = out0;
}

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
   float2 uv = DTid.xy;
   uv.x /= width;
   uv.y /= height;

   float depth = input.GatherRed(g_PointClampSampler, uv, int2(0, 0));
   if (depth == 1.f)
   {
      output[DTid.xy] = 1.f;
      return;
   }
   CSGenerateQ0(DTid.xy);
}
