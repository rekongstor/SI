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

Texture2D diffuseMap : register(t0, space1);
Texture2D materialMap : register(t1, space1);
Texture2D normalMap : register(t2, space1);

SamplerState s1 : register(s0);

PSOutput main(PSInput input) : SV_TARGET
{
   float4 diffuseColor = diffuseMap.Sample(s1, input.uv);
   float2 material = materialMap.Sample(s1, input.uv).zy;
   float3 normalTex = normalMap.Sample(s1, input.uv).xyz * 2.f - 1.f;
   float metalness = material.x;
   float roughness = material.y;

   float3 n = input.normal.xyz;
   float3 t, b;
   float3 c1 = cross(n, float3(1, 0.5, 1));
   float3 c2 = cross(n, float3(0, 1, 0));
   if (length(c1) > length(c2))
      t = c1;
   else
      t = c2;
   t = normalize(t);
   b = normalize(cross(n, t));
   float3x3 tbn = float3x3(t, b, n);
   tbn = 1.f / determinant(tbn) * transpose(tbn);
   normalTex.y *= -1;
   float3 normal = mul(tbn, normalize(normalTex));
   PSOutput output;
   output.color = float4(diffuseColor.xyz, roughness);
   output.normals = float4((normal + 1.f) / 2.f, metalness);
   if (diffuseColor.w < 0.1f)
      discard;
   return output;
}
