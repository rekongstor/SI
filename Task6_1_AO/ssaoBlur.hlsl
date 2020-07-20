SamplerState gPointClampSampler : register(s0);

cbuffer cbPass : register(b0)
{
float4x4 projMatrix;
float width;
float height;
}

Texture2D<float1> ssaoInput : register(t0);
RWTexture2D<float1> ssaoBlurredOutput: register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
   float result = 0.0;
   for (int x = -2; x < 2; ++x)
      for (int y = -2; y < 2; ++y)
      {
         result += ssaoInput.GatherRed(gPointClampSampler, DTid.xy / float2(width, height), int2(x,y));
      }
   ssaoBlurredOutput[DTid.xy] = result / 16.f;
}
