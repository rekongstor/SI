SamplerState g_PointClampSampler : register(s0); // corresponds to SSAO_SAMPLERS_SLOT0
SamplerState g_PointMirrorSampler : register(s1); // corresponds to SSAO_SAMPLERS_SLOT2
SamplerState g_LinearClampSampler : register(s2); // corresponds to SSAO_SAMPLERS_SLOT1
SamplerState g_ViewspaceDepthTapSampler : register(s3); // corresponds to SSAO_SAMPLERS_SLOT3
SamplerState g_ZeroTextureSampler : register(s4);

cbuffer cbPass : register(b0)
{
float4x4 viewMatrixInv;
float4x4 viewMatrix;
float width;
float height;
}

RWTexture2D<float4> output: register(u0);
Texture2D input : register(t0);


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

   output[DTid.xy] = 1.f;
}
