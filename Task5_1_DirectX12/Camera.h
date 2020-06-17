#pragma once
#include "stdafx.h"

class Camera
{
public:
   XMFLOAT4 position;
   XMFLOAT4 target;
   XMFLOAT4 up;

   XMMATRIX projMatrix;
   XMMATRIX viewMatrix;

   Camera(XMFLOAT4 position, XMFLOAT4 target, XMFLOAT4 up, float fovAngle, float aspectRatio);
};
