#include "Mesh.h"

Mesh::Mesh(const XMFLOAT4& position, const XMVECTOR& rotation)
{
   vertices.assign({
      {{-0.5f, 0.5f, -0.5f,1.f}, {1.0f, 0.0f, 0.0f, 1.0f}},
      {{0.5f, -0.5f, -0.5f,1.f}, {1.0f, 0.0f, 1.0f, 1.0f}},
      {{-0.5f, -0.5f, -0.5f,1.f}, {0.0f, 0.0f, 1.0f, 1.0f}},
      {{0.5f, 0.5f, -0.5f,1.f}, {0.0f, 1.0f, 0.0f, 1.0f}},
      {{0.5f, -0.5f, -0.5f,1.f}, {1.0f, 0.0f, 0.0f, 1.0f}},
      {{0.5f, 0.5f, 0.5f,1.f}, {1.0f, 0.0f, 1.0f, 1.0f}},
      {{0.5f, -0.5f, 0.5f,1.f}, {0.0f, 0.0f, 1.0f, 1.0f}},
      {{0.5f, 0.5f, -0.5f,1.f}, {0.0f, 1.0f, 0.0f, 1.0f}},
      {{-0.5f, 0.5f, 0.5f,1.f}, {1.0f, 0.0f, 0.0f, 1.0f}},
      {{-0.5f, -0.5f, -0.5f,1.f}, {1.0f, 0.0f, 1.0f, 1.0f}},
      {{-0.5f, -0.5f, 0.5f,1.f}, {0.0f, 0.0f, 1.0f, 1.0f}},
      {{-0.5f, 0.5f, -0.5f,1.f}, {0.0f, 1.0f, 0.0f, 1.0f}},
      {{0.5f, 0.5f, 0.5f,1.f}, {1.0f, 0.0f, 0.0f, 1.0f}},
      {{-0.5f, -0.5f, 0.5f,1.f}, {1.0f, 0.0f, 1.0f, 1.0f}},
      {{0.5f, -0.5f, 0.5f,1.f}, {0.0f, 0.0f, 1.0f, 1.0f}},
      {{-0.5f, 0.5f, 0.5f,1.f}, {0.0f, 1.0f, 0.0f, 1.0f}},
      {{-0.5f, 0.5f, -0.5f,1.f}, {1.0f, 0.0f, 0.0f, 1.0f}},
      {{0.5f, 0.5f, 0.5f,1.f}, {1.0f, 0.0f, 1.0f, 1.0f}},
      {{0.5f, 0.5f, -0.5f,1.f}, {0.0f, 0.0f, 1.0f, 1.0f}},
      {{-0.5f, 0.5f, 0.5f,1.f}, {0.0f, 1.0f, 0.0f, 1.0f}},
      {{0.5f, -0.5f, 0.5f,1.f}, {1.0f, 0.0f, 0.0f, 1.0f}},
      {{-0.5f, -0.5f, -0.5f,1.f}, {1.0f, 0.0f, 1.0f, 1.0f}},
      {{0.5f, -0.5f, -0.5f,1.f}, {0.0f, 0.0f, 1.0f, 1.0f}},
      {{-0.5f, -0.5f, 0.5f,1.f}, {0.0f, 1.0f, 0.0f, 1.0f}},
   });

   indices.assign({
      0, 1, 2,
      0, 3, 1,

      4, 5, 6,
      4, 7, 5,

      8, 9, 10,
      8, 11, 9,

      12, 13, 14,
      12, 15, 13,

      16, 17, 18,
      16, 19, 17,

      20, 21, 22,
      20, 23, 21,
   });
   translationMatrix = XMMatrixTranslationFromVector(XMVECTOR());
   rotationMatrix = XMMatrixIdentity();
   Move(position);
   Rotate(rotation);
}

void Mesh::Move(XMFLOAT4 newPosition)
{
   XMVECTOR vPos = XMLoadFloat4(&newPosition);
   translationMatrix += XMMatrixTranslationFromVector(vPos);
}

void Mesh::Rotate(XMVECTOR newRotation)
{
   rotationMatrix *= XMMatrixRotationQuaternion(newRotation);
   worldMatrix = rotationMatrix * translationMatrix;
}
