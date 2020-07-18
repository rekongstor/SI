SamplerState g_PointClampSampler : register(s0);

cbuffer cbPass : register(b0)
{
float4x4 pMatrixInv;
float width;
float height;
}

RWTexture2D<float4> output: register(u0);
Texture2D input : register(t0);

float4 ndc2viewspace(float2 uv, float2 bias)
{
   float depth = input.GatherRed(g_PointClampSampler, uv, int2(trunc(bias)));
   float4 targetPos = float4((uv + bias / float2(width, height)) * float2(2, -2) - float2(1, -1), depth, 1.f);
   targetPos = mul(pMatrixInv, targetPos);
   targetPos /= targetPos.w;
   return targetPos;
}


float4 calculateEdges(const float centerZ, const float leftZ, const float rightZ, const float topZ, const float bottomZ)
{
   float4 edgesLRTB = float4(leftZ, rightZ, topZ, bottomZ) - centerZ;
   float4 edgesLRTBSlopeAdjusted = edgesLRTB + edgesLRTB.yxwz;
   edgesLRTB = min(abs(edgesLRTB), abs(edgesLRTBSlopeAdjusted));
   return saturate((1.3 - edgesLRTB / (centerZ * 0.040)));
}


float3 calculateNormal(const float4 edgesLRTB, float3 pixCenterPos, float3 pixLPos, float3 pixRPos, float3 pixTPos, float3 pixBPos)
{
   // Get this pixel's viewspace normal
   float4 acceptedNormals = float4(edgesLRTB.x * edgesLRTB.z, edgesLRTB.z * edgesLRTB.y, edgesLRTB.y * edgesLRTB.w, edgesLRTB.w * edgesLRTB.x);

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

   return pixelNormal;
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
      output[DTid.xy] = 0.f;
      return;
   }

   float3 p_10 = ndc2viewspace(uv, float2(+0.0f, -1.0f));
   float3 p_01 = ndc2viewspace(uv, float2(-1.0f, +0.0f));
   float3 p_11 = ndc2viewspace(uv, float2(+0.0f, +0.0f));
   float3 p_21 = ndc2viewspace(uv, float2(+1.0f, +0.0f));
   float3 p_12 = ndc2viewspace(uv, float2(+0.0f, +1.0f));

   float4 edges_11 = calculateEdges(p_11.z, p_01.z, p_21.z, p_10.z, p_12.z);

   float3 norm_11 = calculateNormal(edges_11, p_11, p_01, p_21, p_10, p_12);

   output[DTid.xy] = float4(norm_11,0.f);
}
