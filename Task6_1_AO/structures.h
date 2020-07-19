#pragma once

struct siVertex
{
   XMFLOAT4 position;
   XMFLOAT4 normal;
   XMFLOAT2 uv;
};

struct mainConstBuff
{
   XMFLOAT4X4 viewMatrix;
   XMFLOAT4X4 projMatrix;
};

struct csConstBuff
{
	XMFLOAT4X4 viewMatrixInv;
	XMFLOAT4X4 viewMatrix;
	float width;
	float height;
};
