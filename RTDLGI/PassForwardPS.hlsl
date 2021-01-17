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

float4 main(VS_OUT input) : SV_TARGET
{
   //float3 outOufRange = uint3(abs(input.posV.xyz) < 1.f);
   //float4 rtColor = (1.f - rtOutput.SampleLevel(linearSampler, (input.posV.xyz) * 0.5f + 0.5f, 0)) * outOufRange.x * outOufRange.y * outOufRange.z;
   //uint3 sbPos = uint3((input.posV.xyz * 0.5 + 0.5) * (float)GI_RESOLUTION);
   float3 normalizedPos = (input.posV.xyz * 0.5 + 0.5) * float(NN_RESOLUTION);
   float3 sbPos = frac(normalizedPos) * 2.f - 1.f;
   uint3 dlgiPos = uint3(normalizedPos);
   uint offset = dlgiPos.x + dlgiPos.y * NN_RESOLUTION + dlgiPos.z * NN_RESOLUTION * NN_RESOLUTION;
   offset *= 7;
   float3 squaredDlgi = float3(dlgiOutput[offset], dlgiOutput[offset + 1], dlgiOutput[offset + 2]);
   float4 linearDlgi = float4(dlgiOutput[offset + 3], dlgiOutput[offset + 4], dlgiOutput[offset + 5], dlgiOutput[offset + 6]);
   float3 color = float3(1,0,0) * max(0.f, dot(normalize(input.normal.xyz), normalize(g_sceneCB.lightDirection.xyz)));
   return float4(color, 1) * float4(step(0, dot(linearDlgi, float4(sbPos, 1.f)) + dot(squaredDlgi, float3(sbPos * sbPos))), 1,1,1);
   //return offset / (NN_RESOLUTION*NN_RESOLUTION*NN_RESOLUTION*7.f);
   return float4(sbPos, 1);
}