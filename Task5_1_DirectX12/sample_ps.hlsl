#define PI 3.14159265f

struct outputVertex
{
   float4 position : SV_POSITION;
   float4 color : COLOR0;
   float4 normal : NORMAL0;
   float4 material : COLOR1;
   float4 view : NORMAL1;
};

cbuffer cbPass : register(b0)
{
float4 cbDirection;
float4 cbColor;
float4x4 viewProj;
float4 ambientColor;
float4 camPos;
}


float4 main(outputVertex input) : SV_TARGET
{
   float4 normDiffuseColor =  pow(input.color, 2.2f);
   //float4 normDiffuseColor =  input.color;

   float4 L = -cbDirection;
   float4 N = normalize(input.normal);
   float4 V = normalize(input.view);
   float4 H = normalize(L + V);

   float dotLH = max(dot(L, H), 0.f);
   float dotNH = max(dot(N, H), 0.f);
   float dotNV = max(dot(N, V), 0.f);
   float dotNL = max(dot(N, L), 0.f);

   float normMetalness = 0.05f + clamp(input.material.x, 0.f, 1.f) * 0.94f;
   float4 F = normMetalness * normDiffuseColor + (1.f - normMetalness * normDiffuseColor) * pow(1.f - dotLH, 5.f);


   float normRoughness = 0.15f + clamp(input.material.y, 0.f, 1.f) * 0.75f;
   float a4 = pow(normRoughness, 4.f);
   float denom = dotNH * dotNH * (a4 - 1.f) + 1.f;

   float NDF = a4 / (PI * denom * denom);

   float r = normRoughness + 1.f;
   float k = r * r / 8.f;
   float geomGGX_NV = dotNV / (dotNV * (1.f - k) + k);
   float geomGGX_NL = dotNL / (dotNL * (1.f - k) + k);
   float G = geomGGX_NV * geomGGX_NL;

   float4 specular = F * NDF * G / max(4.f * dotNV * dotNL, 0.001f);
   float4 kD = (1.f - F) * (1.f - normMetalness);

   float4 color = ambientColor * normDiffuseColor + (normDiffuseColor * kD / PI + specular) * cbColor * dotNL;

   //return color;

   color = color / (color + 1.f);

   return pow(color,1.f/2.2f);
}
