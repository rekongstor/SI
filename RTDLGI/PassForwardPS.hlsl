#define HLSL
#include "HlslCompat.h"
#include "SceneConstBuf.h"


cbuffer g_sceneCB : register(b0)
{
   SceneConstantBuffer g_sceneCB;
}

struct VS_OUT
{
   float4 pos : SV_POSITION;
   float4 normal : NORMAL0;
};

float rand_1_05(in float2 uv)
{
    float2 noise = (frac(sin(dot(uv ,float2(12.9898,78.233)*2.0)) * 43758.5453));
    return abs(noise.x + noise.y) * 0.5;
}

float4 main(VS_OUT input) : SV_TARGET
{
   if (rand_1_05(input.pos.xy) > 0.5) {
      discard;
   }
   return input.normal;
}