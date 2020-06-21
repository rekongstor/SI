struct outputVertex
{
   float4 position : SV_POSITION;
   float2 uv : TEXCOORD;
};

Texture2D postProcessed : register(t4, space1);
SamplerState s1 : register(s0);

float4 main(outputVertex input) : SV_TARGET
{
   float4 color = postProcessed.Sample(s1, input.uv);

   return color;
}
