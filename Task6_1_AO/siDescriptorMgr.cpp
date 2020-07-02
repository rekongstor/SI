#include "siDescriptorMgr.h"

siDescriptorMgr::siDescriptorMgr(uint32_t dsvHeapSize, uint32_t rtvHeapSize, uint32_t cbvSrvUavHeapSize,
                                 uint32_t samplerHeapSize):
   dsvHeapSize(dsvHeapSize),
   rtvHeapSize(rtvHeapSize),
   cbvSrvUavHeapSize(cbvSrvUavHeapSize),
   samplerHeapSize(samplerHeapSize),

   dsvHeapLeft(dsvHeapSize),
   rtvHeapLeft(rtvHeapSize),
   cbvSrvUavHeapLeft(cbvSrvUavHeapSize),
   samplerHeapLeft(samplerHeapSize)
{
}

void siDescriptorMgr::onInit(ID3D12Device* device)
{
   std::cout << "Initializing descriptor heaps..." << std::endl;

   auto heapInit = [&device](ComPtr<ID3D12DescriptorHeap>& heap,
                             CD3DX12_CPU_DESCRIPTOR_HANDLE& cpuHandle,
                             CD3DX12_GPU_DESCRIPTOR_HANDLE& gpuHandle,
                             D3D12_DESCRIPTOR_HEAP_TYPE heapType,
                             uint32_t heapSize)
   {
      HRESULT hr = S_OK;
      D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {
         heapType,
         heapSize,
         D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
         0
      };
      hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&heap));
      assert(hr == S_OK);

      cpuHandle = heap->GetCPUDescriptorHandleForHeapStart();
      gpuHandle = heap->GetGPUDescriptorHandleForHeapStart();
   };

   heapInit(dsvHeap, dsvHandle.first, dsvHandle.second,
            D3D12_DESCRIPTOR_HEAP_TYPE_DSV, dsvHeapSize);

   heapInit(rtvHeap, rtvHandle.first, rtvHandle.second,
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV, rtvHeapSize);

   heapInit(cbvSrvUavHeap, cbvSrvUavHandle.first, cbvSrvUavHandle.second,
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, cbvSrvUavHeapSize);

   heapInit(samplerHeap, samplerHandle.first, samplerHandle.second,
            D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, samplerHeapSize);

   dsvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
   rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
   cbvSrvUavDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
   samplerDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
}

std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> siDescriptorMgr::getDsvHandle()
{
   --dsvHeapLeft;
   assert(dsvHeapLeft >= 0);
   auto ret = dsvHandle;
   dsvHandle.first.Offset(1, dsvDescriptorSize);
   dsvHandle.second.Offset(1, dsvDescriptorSize);
   return ret;
}

std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> siDescriptorMgr::getRtvHandle()
{
   --rtvHeapLeft;
   assert(rtvHeapLeft >= 0);
   auto ret = rtvHandle;
   rtvHandle.first.Offset(1, rtvDescriptorSize);
   rtvHandle.second.Offset(1, rtvDescriptorSize);
   return ret;
}

std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> siDescriptorMgr::getCbvSrvUavHandle()
{
   --cbvSrvUavHeapLeft;
   assert(cbvSrvUavHeapLeft >= 0);
   auto ret = cbvSrvUavHandle;
   cbvSrvUavHandle.first.Offset(1, cbvSrvUavDescriptorSize);
   cbvSrvUavHandle.second.Offset(1, cbvSrvUavDescriptorSize);
   return ret;
}

std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> siDescriptorMgr::getSamplerHandle()
{
   --samplerHeapLeft;
   assert(samplerHeapLeft >= 0);
   auto ret = samplerHandle;
   samplerHandle.first.Offset(1, samplerDescriptorSize);
   samplerHandle.second.Offset(1, samplerDescriptorSize);
   return ret;
}
