#include "cacaoHLSL.hlsl"

Texture2D<float>   g_ImportanceAIn  : register(t0);
RWTexture2D<float> g_ImportanceAOut : register(u0);

static const float cSmoothenImportance = 1.0;

[numthreads(8, 8, 1)]
void main(uint2 tid : SV_DispatchThreadID)
{
	float2 uv = (float2(tid)+0.5f) * g_CACAOConsts.ImportanceMapInverseDimensions;

	float centre = g_ImportanceAIn.SampleLevel(g_LinearClampSampler, uv, 0.0).x;
	//return centre;

	float2 halfPixel = 0.5f * g_CACAOConsts.ImportanceMapInverseDimensions;

	float4 vals;
	vals.x = g_ImportanceAIn.SampleLevel(g_LinearClampSampler, uv + float2(-halfPixel.x * 3, -halfPixel.y), 0.0).x;
	vals.y = g_ImportanceAIn.SampleLevel(g_LinearClampSampler, uv + float2(+halfPixel.x, -halfPixel.y * 3), 0.0).x;
	vals.z = g_ImportanceAIn.SampleLevel(g_LinearClampSampler, uv + float2(+halfPixel.x * 3, +halfPixel.y), 0.0).x;
	vals.w = g_ImportanceAIn.SampleLevel(g_LinearClampSampler, uv + float2(-halfPixel.x, +halfPixel.y * 3), 0.0).x;

	float avgVal = dot(vals, float4(0.25, 0.25, 0.25, 0.25));
	vals.xy = max(vals.xy, vals.zw);
	float maxVal = max(centre, max(vals.x, vals.y));

	g_ImportanceAOut[tid] = lerp(maxVal, avgVal, cSmoothenImportance);
}