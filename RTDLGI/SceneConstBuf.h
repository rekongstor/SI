#ifndef SCENE_CONST_BUFFER_H
#define SCENE_CONST_BUFFER_H

struct SceneConstantBuffer
{
   XMMATRIX viewProj;
   XMMATRIX viewProjInv;
   XMFLOAT4 cameraPosition;
   XMFLOAT4 lightPosition;
   XMFLOAT4 lightDirection;
   XMFLOAT4 lightAmbientColor;
   XMFLOAT4 lightDiffuseColor;
   XMFLOAT4 screenData;
   XMFLOAT4 counter;
};

#ifndef HLSL
#include "rnd_ConstantBuffer.h"

class SceneConstBuf : public SceneConstantBuffer, public rnd_ImmutableConstBuffer<SceneConstantBuffer>
{
public:
   void OnInit(LPCWSTR name) override;
   void Update() override;
};

extern ConstBufInitializer<SceneConstBuf> SCENE_CB;
#endif

#endif
