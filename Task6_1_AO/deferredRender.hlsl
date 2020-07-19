#define PI 3.14159265f

cbuffer cbPass : register(b0)
{
float4 camPos;
float4 lightDirection;
float4 lightColor;
float4 ambientColor;
}

float GGX_PartialGeometry(float cosThetaN, float alpha)
{
   float cosTheta_sqr = saturate(cosThetaN * cosThetaN);
   float tan2 = (1 - cosTheta_sqr) / cosTheta_sqr;
   float GP = 2 / (1 + sqrt(1 + alpha * alpha * tan2));
   return GP;
}


[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{

   //float4 L = -lightDirection;
   //float4 N = normalize(input.normal);
   //float4 V = normalize(input.view);
   //float4 H = normalize(L + V);
   //float dotLH = max(dot(L, H), 0.001f);
   //float dotNH = max(dot(N, H), 0.001f);
   //float dotNV = max(dot(N, V), 0.001f);
   //float dotNL = max(dot(N, L), 0.001f);

   //float normMetalness = 0.05f + clamp(metalness, 0.f, 1.f) * 0.94f;
   //float4 F = normMetalness * normDiffuseColor + (1.f - normMetalness * normDiffuseColor) * pow(1.f - dotLH, 5.f);


   //float normRoughness = 0.05f + clamp(roughness, 0.f, 1.f) * 0.95f;
   //float a4 = pow(normRoughness, 4.f);
   //float denom = dotNH * dotNH * (a4 - 1.f) + 1.f;

   //float NDF = a4 / (PI * denom * denom);

   //float G = GGX_PartialGeometry(dotNV, normRoughness * normRoughness) * GGX_PartialGeometry(
   //   dotNL, normRoughness * normRoughness);

   //float4 specular = F * NDF * G / max(4.f * dotNV * dotNL, 0.001f);
   //float4 kD = (1.f - F) * (1.f - normMetalness);

   //float4 color = ambientColor * normDiffuseColor + (normDiffuseColor * kD / PI + specular) * lightColor * dotNL;

   //color = pow(color / (color + 1.f), 1.f / 2.2f);
}
