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
   float4 posV : POSITION;
   float4 normal : NORMAL0;
};

VS_OUT main(in Vertex v, in uint instId : SV_InstanceID)
{
   VS_OUT output;
   float4x4 instMatr = {
      instanceData[instId].worldMat[0],
      instanceData[instId].worldMat[1],
      instanceData[instId].worldMat[2],
      float4(0,0,0,1)
   };
   float4x4 instMatrInv = {
      instanceData[instId].worldMatInv[0],
      instanceData[instId].worldMatInv[1],
      instanceData[instId].worldMatInv[2],
      float4(0,0,0,1)
   };
   float4 worldPos = mul(instMatr, float4(v.position, 1.f));
   output.pos = mul(g_sceneCB.viewProj, worldPos);
   output.posV = worldPos;
   output.normal = mul(float4(v.normal, 0.f), instMatrInv);
   return output;
}