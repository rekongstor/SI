SamplerState gPointClampSampler : register(s0); // corresponds to SSAO_SAMPLERS_SLOT0
SamplerState gPointMirrorSampler : register(s1); // corresponds to SSAO_SAMPLERS_SLOT2
SamplerState gLinearClampSampler : register(s2); // corresponds to SSAO_SAMPLERS_SLOT1
SamplerState gViewspaceDepthTapSampler : register(s3); // corresponds to SSAO_SAMPLERS_SLOT3
SamplerState gZeroTextureSampler : register(s4);

cbuffer cbPass : register(b0)
{
float4x4 projMatrixInv;
float4 lightDirection;
float4 lightColor;
float4 ambientColor;
float aoPower;
int targetOutput;
float width;
float height;
}

Texture2D diffuseRenderTarget : register(t0);
Texture2D depthStencil : register(t1);
Texture2D normalsRenderTarget : register(t2);
Texture2DArray ssaoOutput : register(t3);
RWTexture2D<float4> deferredRenderTarget: register(u0);

#define PI 3.14159265f

float GGX_PartialGeometry(float cosThetaN, float alpha)
{
   float cosTheta_sqr = saturate(cosThetaN * cosThetaN);
   float tan2 = (1 - cosTheta_sqr) / cosTheta_sqr;
   float GP = 2 / (1 + sqrt(1 + alpha * alpha * tan2));
   return GP;
}

float3 getPosFromNdc(uint2 dTid)
{
   float depthSample = depthStencil.GatherRed(gPointClampSampler, (float2(dTid.xy) + 0.5f) / float2(width, height),
                                              int2(0, 0));
   float4 ndcPos = float4((float2(dTid.xy) + 0.5f) / float2(width, height) * 2.f - 1.f, depthSample, 1.f);
   float4 viewPos = mul(projMatrixInv, ndcPos);
   viewPos.y = -viewPos.y;
   if (depthSample == 1.f)
      viewPos = float4(1, 1, 1, 1);
   return viewPos.xyz / viewPos.w;
}

[numthreads(8, 8, 1)]
void main(uint3 dTid : SV_DispatchThreadID)
{
   float4 diffuse = diffuseRenderTarget[dTid.xy];
   float4 position = float4(getPosFromNdc(dTid.xy), 1.f);
   float4 normal = normalsRenderTarget[dTid.xy];
   float ao = ssaoOutput.SampleLevel(gPointClampSampler, dTid / float3(width, height, 1), 0);
   float roughness = diffuse.w;
   float metalness = normal.w;

   switch (targetOutput)
   {
   case 1:
      deferredRenderTarget[dTid.xy] = diffuse;
      return;
   case 2:
      deferredRenderTarget[dTid.xy] = position;
      return;
   case 3:
      deferredRenderTarget[dTid.xy] = normal;
      return;
   case 4:
      deferredRenderTarget[dTid.xy] = ssaoOutput.SampleLevel(gPointClampSampler, dTid / float3(width, height, 3), 0);
      return;
   case 5:
      deferredRenderTarget[dTid.xy] = metalness;
      return;
   case 6:
      deferredRenderTarget[dTid.xy] = roughness;
      return;
   }

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
   if (targetOutput == 7)
   {
      deferredRenderTarget[dTid.xy] = float4(specular, 1);
      return;
   }
   float3 kD = (1.f - F) * (1.f - normMetalness);

   float3 color = pow(ao, aoPower) * ambientColor * normDiffuseColor +
      (normDiffuseColor * kD / PI + specular) * lightColor * dotNL;

   color = pow(color / (color + 1.f), 1.f / 2.2f);

   deferredRenderTarget[dTid.xy] = float4(color, 1);
}
