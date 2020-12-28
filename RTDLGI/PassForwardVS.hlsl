#define HLSL
#include "HlslCompat.h"
#include "SceneConstBuf.h"


cbuffer g_sceneCB : register(b0)
{
   SceneConstantBuffer g_sceneCB;
}

StructuredBuffer<Instance> instanceData : register(t0, space0);

struct VS_OUT
{
   float4 pos : SV_POSITION;
   float4 normal : NORMAL0;
};

VS_OUT main(in Vertex v, in uint instId : SV_InstanceID)
{
   VS_OUT output;
   float4x4 instMatr = {
      float4(instanceData[instId].worldMat[0].xyzw),
      float4(instanceData[instId].worldMat[1].xyzw),
      float4(instanceData[instId].worldMat[2].xyzw),
      float4(0,0,0, 1.f)
   };
   output.pos = mul(g_sceneCB.viewProj, mul(instMatr, float4(v.position, 1.f)));
   output.normal = float4(v.normal, 0.f);
   return output;
}