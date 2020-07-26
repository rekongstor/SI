#include "cacaoHLSL.hlsl"

RWTexture2D<float> g_ApplyOutput    : register(u0);
Texture2DArray     g_ApplyFinalSSAO : register(t0);

float4 UnpackEdges(float _packedVal)
{
	uint packedVal = (uint)(_packedVal * 255.5);
	float4 edgesLRTB;
	edgesLRTB.x = float((packedVal >> 6) & 0x03) / 3.0;          // there's really no need for mask (as it's an 8 bit input) but I'll leave it in so it doesn't cause any trouble in the future
	edgesLRTB.y = float((packedVal >> 4) & 0x03) / 3.0;
	edgesLRTB.z = float((packedVal >> 2) & 0x03) / 3.0;
	edgesLRTB.w = float((packedVal >> 0) & 0x03) / 3.0;

	return saturate(edgesLRTB + g_CACAOConsts.InvSharpness);
}

[numthreads(8, 8, 1)]
void main(uint2 coord : SV_DispatchThreadID)
{
	float ao;
	float2 inPos = coord;
	uint2 pixPos = coord;
	uint2 pixPosHalf = pixPos / uint2(2, 2);

	// calculate index in the four deinterleaved source array texture
	int mx = (pixPos.x % 2);
	int my = (pixPos.y % 2);
	int ic = mx + my * 2;       // center index
	int ih = (1 - mx) + my * 2;   // neighbouring, horizontal
	int iv = mx + (1 - my) * 2;   // neighbouring, vertical
	int id = (1 - mx) + (1 - my) * 2; // diagonal

	float2 centerVal = g_ApplyFinalSSAO.Load(int4(pixPosHalf, ic, 0)).xy;

	ao = centerVal.x;

#if 1   // change to 0 if you want to disable last pass high-res blur (for debugging purposes, etc.)
	float4 edgesLRTB = UnpackEdges(centerVal.y);

	// return 1.0 - float4( edgesLRTB.x, edgesLRTB.y * 0.5 + edgesLRTB.w * 0.5, edgesLRTB.z, 0.0 ); // debug show edges

	// convert index shifts to sampling offsets
	float fmx = (float)mx;
	float fmy = (float)my;

	// in case of an edge, push sampling offsets away from the edge (towards pixel center)
	float fmxe = (edgesLRTB.y - edgesLRTB.x);
	float fmye = (edgesLRTB.w - edgesLRTB.z);

	// calculate final sampling offsets and sample using bilinear filter
	float2  uvH = (inPos.xy + float2(fmx + fmxe - 0.5, 0.5 - fmy)) * 0.5 * g_CACAOConsts.SSAOBufferInverseDimensions;
	float   aoH = g_ApplyFinalSSAO.SampleLevel(g_LinearClampSampler, float3(uvH, ih), 0).x;
	float2  uvV = (inPos.xy + float2(0.5 - fmx, fmy - 0.5 + fmye)) * 0.5 * g_CACAOConsts.SSAOBufferInverseDimensions;
	float   aoV = g_ApplyFinalSSAO.SampleLevel(g_LinearClampSampler, float3(uvV, iv), 0).x;
	float2  uvD = (inPos.xy + float2(fmx - 0.5 + fmxe, fmy - 0.5 + fmye)) * 0.5 * g_CACAOConsts.SSAOBufferInverseDimensions;
	float   aoD = g_ApplyFinalSSAO.SampleLevel(g_LinearClampSampler, float3(uvD, id), 0).x;

	// reduce weight for samples near edge - if the edge is on both sides, weight goes to 0
	float4 blendWeights;
	blendWeights.x = 1.0;
	blendWeights.y = (edgesLRTB.x + edgesLRTB.y) * 0.75;
	blendWeights.z = (edgesLRTB.z + edgesLRTB.w) * 0.75;
	blendWeights.w = (blendWeights.y + blendWeights.z) * 0.5;

	// calculate weighted average
	float blendWeightsSum = dot(blendWeights, float4(1.0, 1.0, 1.0, 1.0));
	ao = dot(float4(ao, aoH, aoV, aoD), blendWeights) / blendWeightsSum;
#endif

	g_ApplyOutput[coord] = ao.x;
}