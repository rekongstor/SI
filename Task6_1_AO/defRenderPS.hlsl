struct PSInput
{
   float4 position : SV_POSITION;
   float4 pos : POSITION;
   float4 normal : NORMAL0;
   float2 uv : TEXCOORD;
};

struct PSOutput
{
   float4 color : SV_TARGET0;
   float4 position : SV_TARGET1;
   float4 normals : SV_TARGET2;
};

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
   output.position = float4(input.pos.xyz, roughness);
   output.normals = float4(normalize(input.normal).xyz, metalness);
   output.color = diffuseColor;

   return output;
}
