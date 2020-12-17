#ifndef CUBE_CONST_BUF_H
#define CUBE_CONST_BUF_H

struct CubeConstantBuffer
{
   XMFLOAT4 albedo;
};

#ifndef HLSL
#include "rnd_ConstantBuffer.h"

class CubeConstBuf : public CubeConstantBuffer, public rnd_ImmutableConstBuffer<CubeConstantBuffer>
{
   void OnInit(LPCWSTR name) override;
   void Update() override;
};

extern const ConstBufInitializer<CubeConstBuf> CUBE_CB;
#endif

#endif
