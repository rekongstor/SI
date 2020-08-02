#pragma once
#include "cacao.h"

struct siVertex
{
   float4 position;
   float4 normal;
   float2 uv;
};

struct mainConstBuff
{
   float4x4 viewMatrix;
   float4x4 projMatrix;
};

struct ssaoConstBuff
{
   FfxCacaoConstants consts;
};

struct defaultSsaoConstBuff
{
   float4x4 projMatrix;
   float4x4 projMatrixInv;
   float width;
   float height;
   float radius;
   float bias;
   float widthInv;
   float heightInv;
};

struct defRenderConstBuff
{
   float4x4 projMatrixInv;
   float4 lightDirection;
   float4 lightColor;
   float4 ambientColor;
   int targetOutput;
   int targetArray;
   int targetMip;
   float width;
   float height;
};
