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
   float metalness = 1.f; // -metallic.Sample(s1, input.uv).x;
   float roughness = 1.f; // -rough.Sample(s1, input.uv).x;

   float3 normal = input.normal.xyz;
   float3 normalMap = metallic.Sample(s1, input.uv).xyz * 2.f - 1.f;
   //float3 tangent = normalize(float3(0.f, 1.f, 0.f) - normal * dot(float3(0.f, 1.f, 0.f), normal));
   //float3 biTangent = cross(normal, tangent);
   //float3x3 tbn = transpose(float3x3(tangent, biTangent, normal));

   //float3 normalMap = mul(tbn,  input.normal.xyz;
   PSOutput output;
   output.color = float4(diffuseColor.xyz, roughness);
   output.normals = float4((normalize(normal - 0.0*normalMap) + 1.f) / 2.f, metalness);
   if (diffuseColor.w < 0.1f)
      discard;
   return output;
}
