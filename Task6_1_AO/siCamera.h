#pragma once
class siCamera
{
public:
   XMMATRIX projMatrix;
   XMMATRIX viewMatrix;
   XMFLOAT4 position;
   XMFLOAT4 target;
   XMFLOAT4 up;
   float yaw;
   float pitch;
   siCamera(const XMFLOAT4& position, const XMFLOAT4& target, float fovAngle, float aspectRatio, float nearPlane = 0.1f, float farPlane = 1000.f);
   void update();
   void lookAt(float deltaYaw, float deltaPitch);
   void move(char direction, float distance);
};

