#pragma once
#include "stdafx.h"

class Instance
{
public:
   XMMATRIX rotationMatrix;
   XMMATRIX translationMatrix;
   XMMATRIX scaleMatrix;
   XMMATRIX worldMatrix;

   explicit Instance(const XMFLOAT4& position, const XMVECTOR& rotation, const XMFLOAT3& scale);
   explicit Instance(const XMFLOAT4& position, const XMVECTOR& rotation, float scale);

   void move(XMFLOAT4 translation);
   void rotate(XMVECTOR rotation);
   void rescale(float x, float y, float z);
   void rescale(float scale);
};

