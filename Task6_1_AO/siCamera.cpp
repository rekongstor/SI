#include "siCamera.h"

siCamera::siCamera(const XMFLOAT4& position, const XMFLOAT4& target, float fovAngle, float aspectRatio, float nearPlane,
                   float farPlane): position(position),
                                    target(target),
                                    up({0.0001f, 1.0f, 0.0001f, 0.f})
{
   projMatrix = XMMatrixPerspectiveFovLH(fovAngle * (3.14f / 180.f), aspectRatio, nearPlane, farPlane);
}

void siCamera::update()
{
   viewMatrix = XMMatrixLookAtLH(XMLoadFloat4(&position), XMLoadFloat4(&target), XMLoadFloat4(&up));
}
