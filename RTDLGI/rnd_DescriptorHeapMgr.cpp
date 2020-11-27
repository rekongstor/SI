#include "rnd_DescriptorHeapMgr.h"

void rnd_DescriptorHeapMgr::OnInit(ID3D12Device* device)
{
   auto heapInit = [&device](ComPtr<ID3D12DescriptorHeap>& heap,
                             CD3DX12_CPU_DESCRIPTOR_HANDLE& cpuHandle,
                             D3D12_GPU_DESCRIPTOR_HANDLE& gpuHandle,
                             D3D12_DESCRIPTOR_HEAP_TYPE heapType,
                             D3D12_DESCRIPTOR_HEAP_FLAGS flags,
                             uint32_t heapSize)
   {
      D3D12_DESCRIPTOR_HEAP_DESC desc({heapType, heapSize, flags, 0});
      ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));
      cpuHandle = heap->GetCPUDescriptorHandleForHeapStart();
      gpuHandle = heap->GetGPUDescriptorHandleForHeapStart();
   };

   heapInit(dsvHeap, dsvHandle.first, dsvHandle.second,
            D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, dsvCount);
   dsvHeap.Get()->SetName(L"Descriptor heap DSV");

   heapInit(rtvHeap, rtvHandle.first, rtvHandle.second,
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, rtvCount);
   dsvHeap.Get()->SetName(L"Descriptor heap RTV");

   heapInit(cbvSrvUavHeap, cbvSrvUavHandle.first, cbvSrvUavHandle.second,
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, cbvSrvUavCount);
   dsvHeap.Get()->SetName(L"Descriptor heap SrvUav");

   heapInit(svHeap, samplerHandle.first, samplerHandle.second,
            D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, svCount);
   dsvHeap.Get()->SetName(L"Descriptor heap Sampler");

   dsvIncrSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
   rtvIncrSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
   cbvSrvUavIncrSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
   svIncrSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
}

std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> rnd_DescriptorHeapMgr::GetDsvHandle()
{
   --dsvCount;
   assert(dsvCount >= 0);
   auto ret = dsvHandle;
   dsvHandle.first.Offset(1, dsvIncrSize);
   dsvHandle.second.Offset(1, dsvIncrSize);
   return ret;
}

std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> rnd_DescriptorHeapMgr::GetRtvHandle()
{
   --rtvCount;
   assert(rtvHeapLeft >= 0);
   auto ret = rtvHandle;
   rtvHandle.first.Offset(1, rtvIncrSize);
   rtvHandle.second.Offset(1, rtvIncrSize);
   return ret;
}

std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> rnd_DescriptorHeapMgr::GetCbvSrvUavHandle()
{
   --cbvSrvUavCount;
   assert(cbvSrvUavHeapLeft >= 0);
   auto ret = cbvSrvUavHandle;
   cbvSrvUavHandle.first.Offset(1, cbvSrvUavIncrSize);
   cbvSrvUavHandle.second.Offset(1, cbvSrvUavIncrSize);
   return ret;
}

std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> rnd_DescriptorHeapMgr::GetSamplerHandle()
{
   --svCount;
   assert(samplerHeapLeft >= 0);
   auto ret = samplerHandle;
   samplerHandle.first.Offset(1, svIncrSize);
   samplerHandle.second.Offset(1, svIncrSize);
   return ret;
}
