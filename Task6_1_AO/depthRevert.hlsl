SamplerState gPointClampSampler : register(s0); // corresponds to SSAO_SAMPLERS_SLOT0

Texture2D depthStencil : register(t0);
RWTexture2D<float4> depthStencilOut: register(u0);

cbuffer cbPass : register(b0)
{
   float4x4 projMatrixInv;
   float4 lightDirection;
   float4 lightColor;
   float4 ambientColor;
   int targetOutput;
   int targetArray;
   int targetMip;
   float width;
   float height;
}

[numthreads(8, 8, 1)]
void main(uint3 dTid : SV_DispatchThreadID)
{
   float d = depthStencil.GatherRed(gPointClampSampler, float2(dTid.x / width, dTid.y / height));
   depthStencilOut[dTid.xy] = 1.f - (0.1f / d) / 40.f;
}