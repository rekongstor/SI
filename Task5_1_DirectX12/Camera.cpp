#include "Camera.h"

Camera::Camera(XMFLOAT4 position, XMFLOAT4 target, XMFLOAT4 up, float fovAngle, float aspectRatio):
   position(position),
   target(target),
   up(up)
{
   projMatrix = XMMatrixPerspectiveFovLH(
      fovAngle * (3.14f / 180.f),
      aspectRatio,
      0.1f,
      1000.f
   );

   XMVECTOR vPos = XMLoadFloat4(&position);
   XMVECTOR vTarget = XMLoadFloat4(&target);
   XMVECTOR vUp = XMLoadFloat4(&up);
   viewMatrix = XMMatrixLookAtLH(vPos, vTarget, vUp);
}

void Camera::Update()
{
   XMVECTOR vPos = XMLoadFloat4(&position);
   XMVECTOR vTarget = XMLoadFloat4(&target);
   XMVECTOR vUp = XMLoadFloat4(&up);
   viewMatrix = XMMatrixLookAtLH(vPos, vTarget, vUp);
}
