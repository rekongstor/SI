#include "cacaoHLSL.hlsl"

Texture2DArray<float2> g_ImportanceFinalSSAO : register(t0);
RWTexture2D<float>     g_ImportanceOut       : register(u0);

[numthreads(8, 8, 1)]
void main(uint2 tid : SV_DispatchThreadID)
{
	uint2 basePos = tid * 2;

	float2 baseUV = (float2(basePos)+float2(0.5f, 0.5f)) * g_CACAOConsts.SSAOBufferInverseDimensions;

	float avg = 0.0;
	float minV = 1.0;
	float maxV = 0.0;
	[unroll]
	for (int i = 0; i < 4; i++)
	{
		float4 vals = g_ImportanceFinalSSAO.GatherRed(g_PointClampSampler, float3(baseUV, i));

		// apply the same modifications that would have been applied in the main shader
		vals = g_CACAOConsts.EffectShadowStrength * vals;

		vals = 1 - vals;

		vals = pow(saturate(vals), g_CACAOConsts.EffectShadowPow);

		avg += dot(float4(vals.x, vals.y, vals.z, vals.w), float4(1.0 / 16.0, 1.0 / 16.0, 1.0 / 16.0, 1.0 / 16.0));

		maxV = max(maxV, max(max(vals.x, vals.y), max(vals.z, vals.w)));
		minV = min(minV, min(min(vals.x, vals.y), min(vals.z, vals.w)));
	}

	float minMaxDiff = maxV - minV;

	g_ImportanceOut[tid] = pow(saturate(minMaxDiff * 2.0), 0.8);
}