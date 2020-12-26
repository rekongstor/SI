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
   float4 color : COLOR0;
};

VS_OUT main(in Vertex v)
{
   VS_OUT output;
   output.pos = mul(g_sceneCB.viewProj, float4(v.position, 1.f));
   output.normal = normalize(mul(float4(v.normal, 0.f), g_sceneCB.viewProjInv));
   output.color = mul(g_sceneCB.viewProjInv, output.pos);
   output.color.xyz = output.color.xyz / (2.f * output.color.w) + 0.5f;
   return output;
}