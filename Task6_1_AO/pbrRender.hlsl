SamplerState gPointClampSampler : register(s0); // corresponds to SSAO_SAMPLERS_SLOT0
SamplerState g_PointMirrorSampler : register(s1); // corresponds to SSAO_SAMPLERS_SLOT2
SamplerState g_LinearClampSampler : register(s2); // corresponds to SSAO_SAMPLERS_SLOT1
SamplerState g_ViewspaceDepthTapSampler : register(s3); // corresponds to SSAO_SAMPLERS_SLOT3
SamplerState g_ZeroTextureSampler : register(s4);

cbuffer cbPass : register(b0)
{
float4 lightDirection;
float4 lightColor;
float4 ambientColor;
float aoPower;
int targetOutput;
}

Texture2D diffuseRenderTarget : register(t0);
Texture2D positionRenderTarget : register(t1);
Texture2D normalsRenderTarget : register(t2);
Texture2D<float1> ssaoOutput : register(t3);
RWTexture2D<float4> deferredRenderTarget: register(u0);

#define PI 3.14159265f

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
   float4 diffuse = diffuseRenderTarget[DTid.xy];
   float4 position = positionRenderTarget[DTid.xy];
   float4 normal = normalsRenderTarget[DTid.xy];
   float ao = ssaoOutput[DTid.xy];

   float roughness = position.w;
   float metalness = normal.w;

   float3 normDiffuseColor = pow(diffuse, 2.2f);

   float3 L = -lightDirection.xyz;
   float3 N = normal.xyz;
   float3 V = normalize(-position.xyz);
   float3 H = normalize(L + V);
   float dotLH = max(dot(L, H), 0.001f);
   float dotNH = max(dot(N, H), 0.001f);
   float dotNV = max(dot(N, V), 0.001f);
   float dotNL = max(dot(N, L), 0.001f);

   float normMetalness = 0.05f + clamp(metalness, 0.f, 1.f) * 0.94f;
   float3 F = normMetalness * normDiffuseColor + (1.f - normMetalness * normDiffuseColor) * pow(1.f - dotLH, 5.f);

   float normRoughness = 0.05f + clamp(roughness, 0.f, 1.f) * 0.95f;
   float a4 = pow(normRoughness, 4.f);
   float denom = dotNH * dotNH * (a4 - 1.f) + 1.f;

   float NDF = a4 / (PI * denom * denom);

   float G = GGX_PartialGeometry(dotNV, normRoughness * normRoughness) * GGX_PartialGeometry(
      dotNL, normRoughness * normRoughness);

   float3 specular = F * NDF * G / max(4.f * dotNV * dotNL, 0.001f);
   float3 kD = (1.f - F) * (1.f - normMetalness);

   float3 color = pow(ao, aoPower) * ambientColor * normDiffuseColor * kD +
      (normDiffuseColor * kD / PI + specular) * lightColor * dotNL;

   color = pow(color / (color + 1.f), 1.f / 2.2f);
   switch (targetOutput)
   {
   case 0:
      deferredRenderTarget[DTid.xy] = float4(color, 1);
      return;
   case 1:
      deferredRenderTarget[DTid.xy] = diffuse;
      return;
   case 2:
      deferredRenderTarget[DTid.xy] = position;
      return;
   case 3:
      deferredRenderTarget[DTid.xy] = abs(normal);
      return;
   case 4:
      deferredRenderTarget[DTid.xy] = ssaoOutput[DTid.xy];
      return;
   case 5:
      deferredRenderTarget[DTid.xy] = metalness;
      return;
   case 6:
      deferredRenderTarget[DTid.xy] = roughness;
      return;
   case 7:
      deferredRenderTarget[DTid.xy] = float4(specular, 1);
      return;
   }
}
