SamplerState gPointClampSampler : register(s0);
SamplerState gPointMirrorSampler : register(s1);
SamplerState gLinearClampSampler : register(s2);
SamplerState gViewSpaceDepthTapSampler : register(s3);
SamplerState gZeroTextureSampler : register(s4);

cbuffer cbPass : register(b0)
{
float4x4 projMatrixInv;
float width;
float height;
}

Texture2D<float1> depthStencil : register(t0);
Texture2D<float4> normalsRenderTarget : register(t1);
Texture2D<float4> positionRenderTarget : register(t2);
RWTexture2D<float4> ssaoOutput: register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
   float2 uv = DTid.xy;
   uv.x /= width;
   uv.y /= height;

   float depth = depthStencil.GatherRed(gPointClampSampler, uv, int2(0, 0));
   if (depth == 1.f)
   {
      ssaoOutput[DTid.xy] = 1.f;
      return;
   }

   ssaoOutput[DTid.xy] = depth;
}
