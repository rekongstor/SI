#include "cacaoSSAO.hlsl"

[numthreads(8, 8, 1)]
void main(uint2 coord : SV_DispatchThreadID)
{
   float2 inPos = (float2)coord;
   float outShadowTerm;
   float outWeight;
   float4 outEdges;
   GenerateSSAOShadowsInternal(outShadowTerm, outEdges, outWeight, inPos.xy, 3, false);
   float2 out0;
   out0.x = outShadowTerm;
   out0.y = PackEdges(outEdges);
   g_SSAOOutput[int3(coord, g_CACAOConsts.PassIndex)] = out0;
}
