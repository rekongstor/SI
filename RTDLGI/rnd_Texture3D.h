#pragma once
#include "rnd_UploadableBuffer.h"

class rnd_Texture3D : public rnd_UploadableBuffer, public Buffer3D
{
   int mips;
   D3D12_RESOURCE_FLAGS flags;

public:
   DescHandlePair dsvHandle;
   DescHandlePair rtvHandle;
   DescHandlePair srvHandle;
   DescHandlePair uavHandle[MAX_MIP];

   void OnInit(DXGI_FORMAT format, Buffer3D dim, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON, LPCWSTR name = L"", int mips = 1);
   void SetState(D3D12_RESOURCE_STATES nextState);

   void CreateSrv();
   void CreateUav(int mipSlice = 0);

   virtual ~rnd_Texture3D() = default;
};

