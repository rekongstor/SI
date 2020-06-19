#include "Instance.h"

Instance::Instance(const XMFLOAT4& position, const XMVECTOR& rotation, const XMFLOAT3& scale, const XMFLOAT4& material):
   material(material)
{
   translationMatrix = XMMatrixTranslationFromVector(XMVECTOR());
   rotationMatrix = XMMatrixIdentity();
   move(position);
   rotate(rotation);
   rescale(scale.x, scale.y, scale.z);
}

Instance::Instance(const XMFLOAT4& position, const XMVECTOR& rotation, float scale, const XMFLOAT4& material) :
   material(material)
{
   translationMatrix = XMMatrixTranslationFromVector(XMVECTOR());
   rotationMatrix = XMMatrixIdentity();
   move(position);
   rotate(rotation);
   rescale(scale);
}

void Instance::move(XMFLOAT4 translation)
{
   XMVECTOR vPos = XMLoadFloat4(&translation);
   translationMatrix += XMMatrixTranslationFromVector(vPos);
   worldMatrix = (scaleMatrix * rotationMatrix * translationMatrix);
}

void Instance::rotate(XMVECTOR rotation)
{
   rotationMatrix *= XMMatrixRotationQuaternion(rotation);
   worldMatrix = (scaleMatrix * rotationMatrix * translationMatrix);
}

void Instance::rescale(float x, float y, float z)
{
   scaleMatrix = XMMatrixScaling(x, y, z);
   worldMatrix = (scaleMatrix * rotationMatrix * translationMatrix);
}

void Instance::rescale(float scale)
{
   scaleMatrix = XMMatrixScaling(scale, scale, scale);
   worldMatrix = (scaleMatrix * rotationMatrix * translationMatrix);
}

void Instance::setMaterial(XMFLOAT4 mat)
{
   material = mat;
}
