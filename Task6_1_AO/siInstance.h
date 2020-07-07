#pragma once

struct perInstanceData
{
   XMFLOAT4X4 world;
   XMFLOAT4X4 worldIt;
};

class siInstance
{
   std::vector<perInstanceData> data;
   uint8_t* gpuAddress;
   ComPtr<ID3D12Resource> buffer;
   D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress;
public:
   std::vector<perInstanceData>& get() { return data; }
   [[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS getGpuVirtualAddress(uint32_t currentFrame) const { return gpuVirtualAddress; }

   void initBuffer(ID3D12Device* device);
   void gpuCopy();
};
