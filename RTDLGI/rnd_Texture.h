#pragma once

class rnd_Texture : public D3DBuffer
{
public:
   ComPtr<ID3D12Resource> buffer;
   D3D12_RESOURCE_STATES state; // atomic?
   DXGI_FORMAT format;

   DescHandlePair dsvHandle;
   DescHandlePair rtvHandle;
   DescHandlePair srvHandle;
   DescHandlePair uavHandle;

   void OnInit(ID3D12Resource* buffer, DXGI_FORMAT format, D3D12_RESOURCE_STATES initialState, LPCWSTR name = L"");
   ID3D12Resource* Get() { return buffer.Get(); }
   void SetState(D3D12_RESOURCE_STATES nextState);

   void CreateDsv(ID3D12Device* device, DescHandlePair descMgr);
   void CreateRtv(ID3D12Device* device, DescHandlePair descMgr);
   void CreateSrv(ID3D12Device* device, DescHandlePair descMgr);
   void CreateUav(ID3D12Device* device, DescHandlePair descMgr, int32_t mipLevel);
};

