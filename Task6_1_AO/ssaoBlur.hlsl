SamplerState gPointClampSampler : register(s0);

cbuffer cbPass : register(b0)
{
float4x4 projMatrix;
float4x4 projMatrixInv;
float width;
float height;
float radius;
float bias;
}

Texture2D ssaoInput : register(t0);
RWTexture2D<float1> ssaoBlurredOutput: register(u0);

[numthreads(8, 8, 1)]
void main(uint3 dTid : SV_DispatchThreadID)
{
   float result = 0.0;
   for (int x = -2; x < 2; ++x)
      for (int y = -2; y < 2; ++y)
      {
         result += ssaoInput.GatherRed(gPointClampSampler, dTid.xy / float2(width, height), int2(x,y));
      }
   ssaoBlurredOutput[dTid.xy] = result / 16.f;
}
