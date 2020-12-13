#pragma once

class rnd_Texture2D : public D3DBuffer
{
public:
   DescHandlePair dsvHandle;
   DescHandlePair rtvHandle;
   DescHandlePair srvHandle;
   DescHandlePair uavHandle;

   void OnInit(ID3D12Resource* buffer, DXGI_FORMAT format, D3D12_RESOURCE_STATES initialState, LPCWSTR name = L"");
   void SetState(D3D12_RESOURCE_STATES nextState);

   void CreateDsv();
   void CreateRtv();
   void CreateSrv();
   void CreateUav(int32_t mipLevel);
};

