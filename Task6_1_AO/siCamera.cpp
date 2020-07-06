#include "siCamera.h"

siCamera::siCamera(const XMFLOAT4& position, const XMFLOAT4& target, float fovAngle, float aspectRatio, float nearPlane, float farPlane): position(position),
                                                                      target(target),
                                                                      up({0.0001f, 1.0f, 0.0001f, 0.f})
{

   projMatrix = XMMatrixPerspectiveFovLH(fovAngle * (3.14f / 180.f), aspectRatio, nearPlane, farPlane);
}

void siCamera::update()
{
   XMVECTOR vPos = XMLoadFloat4(&position);
   XMVECTOR vTarget = XMLoadFloat4(&target);
   XMVECTOR vUp = XMLoadFloat4(&up);
   XMStoreFloat4x4(&vpMatrix, XMMatrixTranspose(XMMatrixLookAtLH(vPos, vTarget, vUp) * projMatrix));
}
