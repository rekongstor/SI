#pragma once
#include "D3DBuffer.h"

extern float zeroClearValue[];
extern float onesClearValue[];

class rnd_Texture2D : public D3DBuffer, public Buffer2D
{
   int mips;
   int arrSize;
   D3D12_RESOURCE_FLAGS flags;

public:
   DescHandlePair dsvHandle;
   DescHandlePair rtvHandle;
   DescHandlePair srvHandle;
   DescHandlePair uavHandle;

   void OnInit(ID3D12Resource* buffer, D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON, LPCWSTR name = L"");
   void OnInit(DXGI_FORMAT format, Buffer2D dim, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON, LPCWSTR name = L"", int arrSize = 1, int mips = 1, float* clearValue = zeroClearValue);
   void SetState(D3D12_RESOURCE_STATES nextState);

   void CreateDsv();
   void CreateRtv();
   void CreateSrv();
   void CreateUav(int mipSlice = 0, int arrSlice = 0);
};

