#include "cacaoHLSL.h"
Texture2D<float4>        g_PrepareNormalsFromNormalsInput  : register(t0);
RWTexture2DArray<float4> g_PrepareNormalsFromNormalsOutput : register(u0);


float3 DecodeNormal(float3 encodedNormal)
{
	float3 normal = encodedNormal * g_CACAOConsts.NormalsUnpackMul.xxx + g_CACAOConsts.NormalsUnpackAdd.xxx;

#if SSAO_ENABLE_NORMAL_WORLD_TO_VIEW_CONVERSION
	normal = mul(normal, (float3x3)g_CACAOConsts.NormalsWorldToViewspaceMatrix).xyz;
#endif

	// normal = normalize( normal );    // normalize adds around 2.5% cost on High settings but makes little (PSNR 66.7) visual difference when normals are as in the sample (stored in R8G8B8A8_UNORM,
	//                                  // decoded in the shader), however it will likely be required if using different encoding/decoding or the inputs are not normalized, etc.

	return normal;
}

float3 PrepareNormalsFromInputNormalsLoadNormal(int2 pos)
{
	// float3 encodedNormal = g_NormalmapSource.Load(int3(pos, 0)).xyz;
	float3 encodedNormal = g_PrepareNormalsFromNormalsInput.SampleLevel(g_PointClampSampler, (float2(pos)+0.5f) * g_CACAOConsts.OutputBufferInverseDimensions, 0).xyz;
	return DecodeNormal(encodedNormal);
}

[numthreads(8, 8, 1)]
void main(int2 tid : SV_DispatchThreadID)
{
	int2 baseCoord = 2 * tid;
	g_PrepareNormalsFromNormalsOutput[uint3(tid, 0)] = float4(PrepareNormalsFromInputNormalsLoadNormal(baseCoord + int2(0, 0)), 1.0f);
	g_PrepareNormalsFromNormalsOutput[uint3(tid, 1)] = float4(PrepareNormalsFromInputNormalsLoadNormal(baseCoord + int2(1, 0)), 1.0f);
	g_PrepareNormalsFromNormalsOutput[uint3(tid, 2)] = float4(PrepareNormalsFromInputNormalsLoadNormal(baseCoord + int2(0, 1)), 1.0f);
	g_PrepareNormalsFromNormalsOutput[uint3(tid, 3)] = float4(PrepareNormalsFromInputNormalsLoadNormal(baseCoord + int2(1, 1)), 1.0f);
}