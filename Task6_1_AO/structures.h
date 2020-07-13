#pragma once

struct siVertex
{
   XMFLOAT4 position;
   XMFLOAT4 normal;
   XMFLOAT2 uv;
};

struct mainConstBuff
{
   XMFLOAT4X4 vpMatrix;
   XMFLOAT4 camPos;
   XMFLOAT4 lightDirection;
   XMFLOAT4 lightColor;
   XMFLOAT4 ambientColor;
};