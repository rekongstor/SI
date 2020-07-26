#include "cacaoSSAO.hlsl"

[numthreads(8, 8, 1)]
void main(uint2 coord : SV_DispatchThreadID)
{
   float2 inPos = (float2)coord;
   float outShadowTerm;
   float outWeight;
   float4 outEdges;
   GenerateSSAOShadowsInternal(outShadowTerm, outEdges, outWeight, inPos.xy, 3, true);
   float2 out0;
   out0.x = outShadowTerm;
   out0.y = outWeight / ((float)SSAO_ADAPTIVE_TAP_BASE_COUNT * 4.0);
   //0.0; //frac(outWeight / 6.0);// / (float)(SSAO_MAX_TAPS * 4.0);
   g_SSAOOutput[int3(coord, g_CACAOConsts.PassIndex)] = out0;
}
