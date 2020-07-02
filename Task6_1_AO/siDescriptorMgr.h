#pragma once
class siDescriptorMgr
{
   uint32_t dsvHeapSize;
   uint32_t rtvHeapSize;
   uint32_t cbvSrvUavHeapSize;
   uint32_t samplerHeapSize;

   uint32_t dsvHeapLeft;
   uint32_t rtvHeapLeft;
   uint32_t cbvSrvUavHeapLeft;
   uint32_t samplerHeapLeft;

   ComPtr<ID3D12DescriptorHeap> dsvHeap;
   ComPtr<ID3D12DescriptorHeap> rtvHeap;
   ComPtr<ID3D12DescriptorHeap> cbvSrvUavHeap;
   ComPtr<ID3D12DescriptorHeap> samplerHeap;

   std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> dsvHandle;
   std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> rtvHandle;
   std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> cbvSrvUavHandle;
   std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> samplerHandle;

   uint32_t dsvDescriptorSize;
   uint32_t rtvDescriptorSize;
   uint32_t cbvSrvUavDescriptorSize;
   uint32_t samplerDescriptorSize;

public:
   siDescriptorMgr(uint32_t dsvHeapSize, uint32_t rtvHeapSize, uint32_t cbvSrvUavHeapSize, uint32_t samplerHeapSize);

   void onInit(ID3D12Device* device);

   std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> getDsvHandle();
   std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> getRtvHandle();
   std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> getCbvSrvUavHandle();
   std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> getSamplerHandle();
};

