#include "cacaoHLSL.h"
Texture2D g_DepthSource : register(t0);
RWTexture2DArray<float> g_PrepareDepthsAndMips_OutMip0: register(u0);
RWTexture2DArray<float> g_PrepareDepthsAndMips_OutMip1: register(u1);
RWTexture2DArray<float> g_PrepareDepthsAndMips_OutMip2: register(u2);
RWTexture2DArray<float> g_PrepareDepthsAndMips_OutMip3: register(u3);

groupshared float s_PrepareDepthsAndMipsBuffer[4][8][8];

min16float MipSmartAverage(min16float4 depths)
{
	min16float closest = min(min(depths.x, depths.y), min(depths.z, depths.w));
	min16float falloffCalcMulSq = -1.0f / g_CACAOConsts.EffectRadius * g_CACAOConsts.EffectRadius;
	min16float4 dists = depths - closest.xxxx;
	min16float4 weights = saturate(dists * dists * falloffCalcMulSq + 1.0);
	return dot(weights, depths) / dot(weights, float4(1.0, 1.0, 1.0, 1.0));
}

float4 ScreenSpaceToViewSpaceDepth(float4 screenDepth)
{
	float depthLinearizeMul = g_CACAOConsts.DepthUnpackConsts.x;
	float depthLinearizeAdd = g_CACAOConsts.DepthUnpackConsts.y;

	// Optimised version of "-cameraClipNear / (cameraClipFar - projDepth * (cameraClipFar - cameraClipNear)) * cameraClipFar"

	// Set your depthLinearizeMul and depthLinearizeAdd to:
	// depthLinearizeMul = ( cameraClipFar * cameraClipNear) / ( cameraClipFar - cameraClipNear );
	// depthLinearizeAdd = cameraClipFar / ( cameraClipFar - cameraClipNear );

	return depthLinearizeMul / (depthLinearizeAdd - screenDepth);
}

void PrepareDepthsAndMips(float4 samples, int2 outputCoord, uint2 gtid)
{
	samples = ScreenSpaceToViewSpaceDepth(samples);

	s_PrepareDepthsAndMipsBuffer[0][gtid.x][gtid.y] = samples.w;
	s_PrepareDepthsAndMipsBuffer[1][gtid.x][gtid.y] = samples.z;
	s_PrepareDepthsAndMipsBuffer[2][gtid.x][gtid.y] = samples.x;
	s_PrepareDepthsAndMipsBuffer[3][gtid.x][gtid.y] = samples.y;

	g_PrepareDepthsAndMips_OutMip0[int3(outputCoord.x, outputCoord.y, 0)] = samples.w;
	g_PrepareDepthsAndMips_OutMip0[int3(outputCoord.x, outputCoord.y, 1)] = samples.z;
	g_PrepareDepthsAndMips_OutMip0[int3(outputCoord.x, outputCoord.y, 2)] = samples.x;
	g_PrepareDepthsAndMips_OutMip0[int3(outputCoord.x, outputCoord.y, 3)] = samples.y;

	int depthArrayIndex = 2 * (gtid.y % 2) + (gtid.x % 2);
	int2 depthArrayOffset = int2(gtid.x % 2, gtid.y % 2);
	int2 bufferCoord = gtid - depthArrayOffset;

	outputCoord /= 2;

	// if (stillAlive) <-- all threads alive here
	{
		float sample_00 = s_PrepareDepthsAndMipsBuffer[depthArrayIndex][bufferCoord.x + 0][bufferCoord.y + 0];
		float sample_01 = s_PrepareDepthsAndMipsBuffer[depthArrayIndex][bufferCoord.x + 0][bufferCoord.y + 1];
		float sample_10 = s_PrepareDepthsAndMipsBuffer[depthArrayIndex][bufferCoord.x + 1][bufferCoord.y + 0];
		float sample_11 = s_PrepareDepthsAndMipsBuffer[depthArrayIndex][bufferCoord.x + 1][bufferCoord.y + 1];

		float avg = MipSmartAverage(float4(sample_00, sample_01, sample_10, sample_11));
		g_PrepareDepthsAndMips_OutMip1[int3(outputCoord.x, outputCoord.y, depthArrayIndex)] = avg;
		s_PrepareDepthsAndMipsBuffer[depthArrayIndex][bufferCoord.x][bufferCoord.y] = avg;
	}

	bool stillAlive = gtid.x % 4 == depthArrayOffset.x && gtid.y % 4 == depthArrayOffset.y;

	outputCoord /= 2;
	GroupMemoryBarrierWithGroupSync();

	if (stillAlive)
	{
		float sample_00 = s_PrepareDepthsAndMipsBuffer[depthArrayIndex][bufferCoord.x + 0][bufferCoord.y + 0];
		float sample_01 = s_PrepareDepthsAndMipsBuffer[depthArrayIndex][bufferCoord.x + 0][bufferCoord.y + 2];
		float sample_10 = s_PrepareDepthsAndMipsBuffer[depthArrayIndex][bufferCoord.x + 2][bufferCoord.y + 0];
		float sample_11 = s_PrepareDepthsAndMipsBuffer[depthArrayIndex][bufferCoord.x + 2][bufferCoord.y + 2];

		float avg = MipSmartAverage(float4(sample_00, sample_01, sample_10, sample_11));
		g_PrepareDepthsAndMips_OutMip2[int3(outputCoord.x, outputCoord.y, depthArrayIndex)] = avg;
		s_PrepareDepthsAndMipsBuffer[depthArrayIndex][bufferCoord.x][bufferCoord.y] = avg;
	}

	stillAlive = gtid.x % 8 == depthArrayOffset.x && depthArrayOffset.y % 8 == depthArrayOffset.y;

	outputCoord /= 2;
	GroupMemoryBarrierWithGroupSync();

	if (stillAlive)
	{
		float sample_00 = s_PrepareDepthsAndMipsBuffer[depthArrayIndex][bufferCoord.x + 0][bufferCoord.y + 0];
		float sample_01 = s_PrepareDepthsAndMipsBuffer[depthArrayIndex][bufferCoord.x + 0][bufferCoord.y + 4];
		float sample_10 = s_PrepareDepthsAndMipsBuffer[depthArrayIndex][bufferCoord.x + 4][bufferCoord.y + 0];
		float sample_11 = s_PrepareDepthsAndMipsBuffer[depthArrayIndex][bufferCoord.x + 4][bufferCoord.y + 4];

		float avg = MipSmartAverage(float4(sample_00, sample_01, sample_10, sample_11));
		g_PrepareDepthsAndMips_OutMip3[int3(outputCoord.x, outputCoord.y, depthArrayIndex)] = avg;
	}
}

[numthreads(8, 8, 1)]
void main(uint2 tid : SV_DispatchThreadID, uint2 gtid : SV_GroupThreadID)
{
	int2 depthBufferCoord = 2 * tid.xy;
	int2 outputCoord = tid;

	float2 uv = (float2(depthBufferCoord)+0.5f) * g_CACAOConsts.DepthBufferInverseDimensions;
	float4 samples = g_DepthSource.GatherRed(g_PointClampSampler, uv);

	PrepareDepthsAndMips(samples, outputCoord, gtid);
}
