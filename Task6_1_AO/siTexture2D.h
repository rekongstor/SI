#pragma once
class siDescriptorMgr;

class siTexture2D
{
   ComPtr<ID3D12Resource> buffer;

   std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> dsvHandle;
   std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> rtvHandle;
   std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> srvHandle;
   std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> uavHandle;

public:
   void initFromBuffer(ComPtr<ID3D12Resource>& existingBuffer);
   void initDepthStencil(ID3D12Device* device, uint32_t width, uint32_t height);

   void createDsv(ID3D12Device* device, siDescriptorMgr* descMgr);
   void createRtv(ID3D12Device* device, siDescriptorMgr* descMgr);
   void createSrv(ID3D12Device* device, siDescriptorMgr* descMgr);
   void createUav(ID3D12Device* device, siDescriptorMgr* descMgr);
};
