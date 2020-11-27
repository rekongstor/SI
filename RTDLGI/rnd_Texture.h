#pragma once
#include "rnd_DescriptorHeapMgr.h"

class rnd_Texture
{
public:
   ComPtr<ID3D12Resource> buffer;
   DXGI_FORMAT format;
   D3D12_RESOURCE_STATES state; // atomic?

   std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> dsvHandle;
   std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> rtvHandle;
   std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> srvHandle;
   std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> uavHandle;

   void OnInit(ID3D12Resource* buffer, DXGI_FORMAT format, D3D12_RESOURCE_STATES initialState, LPCWSTR name = L"");
   ID3D12Resource* Get() { return buffer.Get(); }
   void SetState(D3D12_RESOURCE_STATES nextState);

   void CreateDsv(ID3D12Device* device, rnd_DescriptorHeapMgr* descMgr);
   void CreateRtv(ID3D12Device* device, rnd_DescriptorHeapMgr* descMgr);
   void CreateSrv(ID3D12Device* device, rnd_DescriptorHeapMgr* descMgr);
   void CreateUav(ID3D12Device* device, rnd_DescriptorHeapMgr* descMgr, int32_t mipLevel);
};

