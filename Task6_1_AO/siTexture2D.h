#pragma once
#include "siCommandList.h"
class siDescriptorMgr;

class siTexture2D
{
   ComPtr<ID3D12Resource> buffer;
   ComPtr<ID3D12Resource> textureUploadHeap;
   std::vector<BYTE> data;

   DXGI_FORMAT format;
   DXGI_SAMPLE_DESC sampleDesc;
   D3D12_RESOURCE_STATES state;

   std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> dsvHandle;
   std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> rtvHandle;
   std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> srvHandle;
   std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> uavHandle;

public:
   void initFromTexture(const siTexture2D& other);
   void initFromBuffer(ComPtr<ID3D12Resource>& existingBuffer, DXGI_FORMAT format, DXGI_SAMPLE_DESC sampleDesc);
   void initDepthStencil(ID3D12Device* device, uint32_t width, uint32_t height);
   void initTexture(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format,
                    D3D12_RESOURCE_FLAGS flags,
                    D3D12_RESOURCE_STATES initState, DXGI_SAMPLE_DESC sampleDesc);
   void initFromFile(ID3D12Device* device, std::string_view filename, const siCommandList& commandList);
   void releaseUploadBuffer();

   void createDsv(ID3D12Device* device, siDescriptorMgr* descMgr);
   void createRtv(ID3D12Device* device, siDescriptorMgr* descMgr);
   void createSrv(ID3D12Device* device, siDescriptorMgr* descMgr);
   void createUav(ID3D12Device* device, siDescriptorMgr* descMgr);

   [[nodiscard]] const ComPtr<ID3D12Resource>& getBuffer() const;
   [[nodiscard]] const std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE>& getDsvHandle() const;
   [[nodiscard]] const std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE>& getRtvHandle() const;
   [[nodiscard]] const std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE>& getSrvHandle() const;
   [[nodiscard]] const std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE>& getUavHandle() const;

   [[nodiscard]] D3D12_RESOURCE_STATES getState() const { return state; }
   void setState(const D3D12_RESOURCE_STATES state) { this->state = state; }
};
