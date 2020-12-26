#define HLSL
#include "HlslCompat.h"
#include "SceneConstBuf.h"


cbuffer g_sceneCB : register(b0)
{
   SceneConstantBuffer g_sceneCB;
}

struct Vertex
{
    float3 position : POSITION;
    float3 normal : NORMAL0;
};

struct VS_OUT
{
   float4 pos : SV_POSITION;
   float4 normal : NORMAL0;
};

VS_OUT main(in Vertex v)
{
   VS_OUT output;
   output.pos = mul(g_sceneCB.viewProj, float4(v.position, 1.f));
   output.normal = output.pos * 0.5 + 0.5;
   return output;
}