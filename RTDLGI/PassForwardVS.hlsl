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

VS_OUT main(in Vertex v)
{
   VS_OUT output;
   output.pos = mul(g_sceneCB.viewProj, float4(v.position, 1.f));
   output.normal = float4(v.normal, 0.f);
   return output;
}