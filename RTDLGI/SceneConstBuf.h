#ifndef SCENE_CONST_BUFFER_H
#define SCENE_CONST_BUFFER_H

struct SceneConstantBuffer
{
   XMMATRIX viewProj;
   XMMATRIX viewProjInv;
   XMFLOAT3 cameraPosition;
   XMFLOAT3 lightPosition;
   XMFLOAT3 lightAmbientColor;
   XMFLOAT3 lightDiffuseColor;
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
