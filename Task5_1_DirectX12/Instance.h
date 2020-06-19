#pragma once
#include "stdafx.h"

class Instance
{
public:
   XMMATRIX rotationMatrix;
   XMMATRIX translationMatrix;
   XMMATRIX scaleMatrix;
   XMMATRIX worldMatrix;

   XMFLOAT4 material;

   explicit Instance(const XMFLOAT4& position, const XMVECTOR& rotation, const XMFLOAT3& scale, const XMFLOAT4& material);
   explicit Instance(const XMFLOAT4& position, const XMVECTOR& rotation, float scale, const XMFLOAT4& material);

   void move(XMFLOAT4 translation);
   void rotate(XMVECTOR rotation);
   void rescale(float x, float y, float z);
   void rescale(float scale);

   void setMaterial(XMFLOAT4 mat);
};

