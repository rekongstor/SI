#define HLSL
#include "HlslCompat.h"
#include "SceneConstBuf.h"

cbuffer g_sceneCB : register(b0)
{
   SceneConstantBuffer g_sceneCB;
}

Texture3D rtOutput : register(t1);
StructuredBuffer<half> dlgiOutput : register(t2);

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
   float3 outOufRange = uint3(abs(input.posV.xyz) < 1.f);
   float4 rtColor = (1.f - rtOutput.SampleLevel(linearSampler, (input.posV.xyz) * 0.5f + 0.5f, 0)) * outOufRange.x * outOufRange.y * outOufRange.z;
   //uint3 sbPos = uint3((input.posV.xyz * 0.5 + 0.5) * (float)GI_RESOLUTION);
   //float4 rtColor = (dlgiOutput[uint((sbPos.x * GI_RESOLUTION + sbPos.y + sbPos.z * GI_RESOLUTION * GI_RESOLUTION))]).xxxx * 0.5 + 0.5;
   float3 color = float3(1,0,0) * max(0.f, dot(normalize(input.normal.xyz), normalize(g_sceneCB.lightDirection.xyz)));
   return float4(color, 1) * (1.f - rtColor);
}