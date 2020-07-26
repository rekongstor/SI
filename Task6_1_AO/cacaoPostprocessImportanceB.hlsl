#include "cacaoHLSL.hlsl"

Texture2D<float>   g_ImportanceBIn          : register(t0);
RWTexture2D<float> g_ImportanceBOut         : register(u0);
RWTexture1D<uint>  g_ImportanceBLoadCounter : register(u1);

static const float cSmoothenImportance = 1.0;

[numthreads(8, 8, 1)]
void main(uint2 tid : SV_DispatchThreadID)
{
	float2 uv = (float2(tid)+0.5f) * g_CACAOConsts.ImportanceMapInverseDimensions;

	float centre = g_ImportanceBIn.SampleLevel(g_LinearClampSampler, uv, 0.0).x;
	//return centre;

	float2 halfPixel = 0.5f * g_CACAOConsts.ImportanceMapInverseDimensions;

	float4 vals;
	vals.x = g_ImportanceBIn.SampleLevel(g_LinearClampSampler, uv + float2(-halfPixel.x, -halfPixel.y * 3), 0.0).x;
	vals.y = g_ImportanceBIn.SampleLevel(g_LinearClampSampler, uv + float2(+halfPixel.x * 3, -halfPixel.y), 0.0).x;
	vals.z = g_ImportanceBIn.SampleLevel(g_LinearClampSampler, uv + float2(+halfPixel.x, +halfPixel.y * 3), 0.0).x;
	vals.w = g_ImportanceBIn.SampleLevel(g_LinearClampSampler, uv + float2(-halfPixel.x * 3, +halfPixel.y), 0.0).x;

	float avgVal = dot(vals, float4(0.25, 0.25, 0.25, 0.25));
	vals.xy = max(vals.xy, vals.zw);
	float maxVal = max(centre, max(vals.x, vals.y));

	float retVal = lerp(maxVal, avgVal, cSmoothenImportance);
	g_ImportanceBOut[tid] = retVal;

	// sum the average; to avoid overflowing we assume max AO resolution is not bigger than 16384x16384; so quarter res (used here) will be 4096x4096, which leaves us with 8 bits per pixel 
	uint sum = (uint)(saturate(retVal) * 255.0 + 0.5);

	// save every 9th to avoid InterlockedAdd congestion - since we're blurring, this is good enough; compensated by multiplying LoadCounterAvgDiv by 9
	if (((tid.x % 3) + (tid.y % 3)) == 0)
	{
		InterlockedAdd(g_ImportanceBLoadCounter[0], sum);
	}
}