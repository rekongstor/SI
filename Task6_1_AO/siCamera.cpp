#include "siCamera.h"

XMVECTOR PolarToVector(float yaw, float pitch)
{
   return XMVector4Normalize(XMVectorSet(sinf(yaw) * cosf(pitch), sinf(pitch), cosf(yaw) * cosf(pitch), 0));
}

XMMATRIX LookAtRH(XMVECTOR eyePos, XMVECTOR lookAt)
{
   return XMMatrixLookAtRH(eyePos, lookAt, XMVectorSet(0, 1, 0, 0));
}

siCamera::siCamera(const XMFLOAT4& position, const XMFLOAT4& target, float fovAngle, float aspectRatio, float nearPlane,
                   float farPlane): position(position),
                                    target(target),
                                    up({0.0001f, -1.0f, 0.0001f, 0.f}),
                                    yaw(0.f), pitch(0.f)
{
   projMatrix = XMMatrixPerspectiveFovLH(fovAngle * (3.14f / 180.f), aspectRatio, nearPlane, farPlane);
}

void siCamera::update()
{
   viewMatrix = XMMatrixLookAtLH(XMLoadFloat4(&position), XMLoadFloat4(&target), XMLoadFloat4(&up));
}

void siCamera::lookAt(float deltaYaw, float deltaPitch)
{
   yaw += deltaYaw;
   pitch += deltaPitch;
   XMVECTOR eyePos = XMLoadFloat4(&position);
   XMVECTOR dir = PolarToVector(yaw, pitch);
   XMStoreFloat4(&target, eyePos - dir);
}

void siCamera::move(char direction, float distance)
{
   XMVECTOR eyePos = XMLoadFloat4(&position);
   XMFLOAT4 dir = {};
   switch (direction)
   {
   case 'w': // forward
      dir = { 0,0,1,0 };
      break;
   case 's': // back
      dir = { 0,0,-1,0 };
      break;
   case 'a': // left
      dir = { 1, 0, 0,0 };
      break;
   case 'd': // right
      dir = { -1,0,0,0 };
      break;
   }
   XMStoreFloat4(&position, eyePos + XMVector4Transform(XMLoadFloat4(&dir) * distance, XMMatrixTranspose(viewMatrix)));
}
