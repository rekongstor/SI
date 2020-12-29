#define HLSL
#include "HlslCompat.h"
#include "SceneConstBuf.h"

cbuffer g_sceneCB : register(b0)
{
   SceneConstantBuffer g_sceneCB;
}

Texture3D rtOutput : register(t1);

SamplerState pointClampSampler : register(s0);

float rand_1_05(in float2 uv)
{
    float2 noise = (frac(sin(dot(uv ,float2(12.9898,78.233)*2.0)) * 43758.5453));
    return abs(noise.x + noise.y) * 0.5;
}

struct VS_OUT
{
   float4 pos : SV_POSITION;
   float4 posV : POSITION;
   float4 normal : NORMAL0;
};

float4 main(VS_OUT input) : SV_TARGET
{
   //if (rand_1_05(input.pos.xy + input.normal.xy) > 0.5) {
   //   discard;
   //}
   // rtOutput.SampleLevel(pointClampSampler, input.pos.xy * g_sceneCB.screenData.zw, 0);
   float4 rtColor = rtOutput.SampleLevel(pointClampSampler, (input.posV + 1.f) * 0.5f + normalize(g_sceneCB.lightDirection) * 0.f, 0);
   float3 color = float3(1,0,0) * max(0.f, dot(normalize(input.normal.xyz), normalize(g_sceneCB.lightDirection.xyz)));
   return float4(color, 1) * rtColor;
}