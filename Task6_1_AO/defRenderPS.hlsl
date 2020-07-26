struct PSInput
{
   float4 position : SV_POSITION;
   float4 normal : NORMAL0;
   float2 uv : TEXCOORD;
};

struct PSOutput
{
   float4 color : SV_TARGET0;
   float4 normals : SV_TARGET1;
};

cbuffer cbPass : register(b0)
{
   float4x4 viewMatrix;
   float4x4 projMatrix;
}

Texture2D albedo : register(t0, space1);
Texture2D metallic : register(t1, space1);
Texture2D rough : register(t2, space1);

SamplerState s1 : register(s0);

PSOutput main(PSInput input) : SV_TARGET
{
   float4 diffuseColor = albedo.Sample(s1, input.uv);
   float metalness = 1.f - metallic.Sample(s1, input.uv).x;
   float roughness = 1.f - rough.Sample(s1, input.uv).x;

   PSOutput output;
   output.color = float4(diffuseColor.xyz, roughness);
   float4 norm = mul(projMatrix, input.normal);
   norm /= norm.w;
   output.normals = float4(input.normal.xyz, metalness);
   if (diffuseColor.w < 0.1f)
      discard;
   return output;
}
