#pragma once
#include "stdafx.h"
#include <wtypes.h>
#include <vector>


class Mesh
{
public:
   XMMATRIX rotationMatrix;
   XMMATRIX translationMatrix;
   XMMATRIX worldMatrix;

   std::vector<Vertex> vertices;
   std::vector<DWORD> indices;

   explicit Mesh(const XMFLOAT4& position, const XMVECTOR& rotation);
   void Move(XMFLOAT4 newPosition);
   void Rotate(XMVECTOR newRotation);
};