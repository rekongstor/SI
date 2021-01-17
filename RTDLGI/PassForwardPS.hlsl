#define HLSL
#include "HlslCompat.h"
#include "SceneConstBuf.h"

cbuffer g_sceneCB : register(b0)
{
   SceneConstantBuffer g_sceneCB;
}

Texture3D rtOutput : register(t1);
StructuredBuffer<float> dlgiOutput : register(t2);

SamplerState pointSampler : register(s0);
SamplerState linearSampler : register(s1);

struct VS_OUT
{
   float4 pos : SV_POSITION;
   float4 posV : POSITION;
   float4 normal : NORMAL0;
};

float4 linearDLGI(uint3 dlgiPos)
{
   uint offset = dlgiPos.x + dlgiPos.y * NN_RESOLUTION + dlgiPos.z * NN_RESOLUTION * NN_RESOLUTION;
   offset *= 4;
   
   return float4(dlgiOutput[offset + 0], dlgiOutput[offset + 1], dlgiOutput[offset + 2], dlgiOutput[offset + 3]);
}

float4 main(VS_OUT input) : SV_TARGET
{
   //float3 outOufRange = uint3(abs(input.posV.xyz) < 1.f);
   //float4 rtColor = (1.f - rtOutput.SampleLevel(linearSampler, (input.posV.xyz) * 0.5f + 0.5f, 0)) * outOufRange.x * outOufRange.y * outOufRange.z;
   //uint3 sbPos = uint3((input.posV.xyz * 0.5 + 0.5) * (float)GI_RESOLUTION);
   float3 normalizedPos = ((input.posV.xyz + 0.1 * normalize(input.normal)) * 0.5 + 0.5) * float(NN_RESOLUTION);
   float3 sbPos = frac(normalizedPos) * 2 - 1;

   uint3 dlgiPos = uint3(normalizedPos);
   uint3 dlgiLerp = step(0, sbPos);

   float3 color = float3(1,0,0) * max(0.f, dot(normalize(input.normal.xyz), normalize(g_sceneCB.lightDirection.xyz)));

   uint3 pos000 = uint3(0, 0, 0);
   uint3 posx00 = uint3(dlgiLerp.x, 0, 0);
   uint3 pos0y0 = uint3(0, dlgiLerp.y, 0);
   uint3 posxy0 = uint3(dlgiLerp.x, dlgiLerp.y, 0);
   uint3 pos00z = uint3(0, 0, dlgiLerp.z);
   uint3 posx0z = uint3(dlgiLerp.x, 0, dlgiLerp.z);
   uint3 pos0yz = uint3(0, dlgiLerp.y, dlgiLerp.z);
   uint3 posxyz = uint3(dlgiLerp.x, dlgiLerp.y, dlgiLerp.z);

   float3 gPos000 = length(float3(normalizedPos - pos000) - float3(dlgiPos)); 
   float3 gPosx00 = length(float3(normalizedPos - posx00) - float3(dlgiPos)); 
   float3 gPos0y0 = length(float3(normalizedPos - pos0y0) - float3(dlgiPos)); 
   float3 gPosxy0 = length(float3(normalizedPos - posxy0) - float3(dlgiPos)); 
   float3 gPos00z = length(float3(normalizedPos - pos00z) - float3(dlgiPos)); 
   float3 gPosx0z = length(float3(normalizedPos - posx0z) - float3(dlgiPos)); 
   float3 gPos0yz = length(float3(normalizedPos - pos0yz) - float3(dlgiPos)); 
   float3 gPosxyz = length(float3(normalizedPos - posxyz) - float3(dlgiPos));

   float total = 
      + gPos000
      + gPosx00
      + gPos0y0
      + gPosxy0
      + gPos00z
      + gPosx0z
      + gPos0yz
      + gPosxyz
   ;

   float l000 = dot(float4(sbPos - (float3)pos000, 1.f), linearDLGI(dlgiPos + pos000)) * gPos000 / total;
   float lx00 = dot(float4(sbPos - (float3)posx00, 1.f), linearDLGI(dlgiPos + posx00)) * gPosx00 / total;
   float l0y0 = dot(float4(sbPos - (float3)pos0y0, 1.f), linearDLGI(dlgiPos + pos0y0)) * gPos0y0 / total;
   float lxy0 = dot(float4(sbPos - (float3)posxy0, 1.f), linearDLGI(dlgiPos + posxy0)) * gPosxy0 / total;
   float l00z = dot(float4(sbPos - (float3)pos00z, 1.f), linearDLGI(dlgiPos + pos00z)) * gPos00z / total;
   float lx0z = dot(float4(sbPos - (float3)posx0z, 1.f), linearDLGI(dlgiPos + posx0z)) * gPosx0z / total;
   float l0yz = dot(float4(sbPos - (float3)pos0yz, 1.f), linearDLGI(dlgiPos + pos0yz)) * gPos0yz / total;
   float lxyz = dot(float4(sbPos - (float3)posxyz, 1.f), linearDLGI(dlgiPos + posxyz)) * gPosxyz / total;

   float dlgi =
      + l000
      + lx00
      + l0y0
      + lxy0
      + l00z
      + lx0z
      + l0yz
      + lxyz
      ;

   return float4(color, 1) * float4(smoothstep(0, 1, dlgi), 1,1,1);
   //return float4(color, 1) * float4(lerp(tanh(dot(linearDlgi, float4(sbPos, 1.f))), tanh(dot(linearDlgi, float4(sbPos + nearest, 1.f))), length(sbPos)), 1,1,1);
   //return offset / (NN_RESOLUTION*NN_RESOLUTION*NN_RESOLUTION*7.f);
   //return float4(sbPos * 0.5 + 0.5, 1);
}