#pragma once
#include "rnd_UploadableBuffer.h"

extern float zeroClearValue[];
extern float onesClearValue[];

class rnd_Texture2D : public rnd_UploadableBuffer, public Buffer2D
{
   int mips;
   D3D12_RESOURCE_FLAGS flags;

public:
   DescHandlePair dsvHandle;
   DescHandlePair rtvHandle;
   DescHandlePair srvHandle;
   DescHandlePair uavHandle[MAX_MIP];

   void OnInit(ID3D12Resource* buffer, D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON, LPCWSTR name = L"");
   void OnInit(DXGI_FORMAT format, Buffer2D dim, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON, LPCWSTR name = L"", int mips = 1, float* clearValue = zeroClearValue);
   void OnInitReadback(rnd_Texture2D defaultTex, D3D12_RESOURCE_STATES initialState, LPCWSTR name);
   void SetState(D3D12_RESOURCE_STATES nextState);

   void CreateDsv();
   void CreateRtv();
   void CreateSrv();
   void CreateUav(int mipSlice = 0);

   virtual ~rnd_Texture2D() = default;
};

