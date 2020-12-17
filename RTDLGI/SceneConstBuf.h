#ifndef SCENE_CONST_BUFFER_H
#define SCENE_CONST_BUFFER_H

struct SceneConstantBuffer
{
   XMMATRIX projectionToWorld;
   XMVECTOR cameraPosition;
   XMVECTOR lightPosition;
   XMVECTOR lightAmbientColor;
   XMVECTOR lightDiffuseColor;
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
