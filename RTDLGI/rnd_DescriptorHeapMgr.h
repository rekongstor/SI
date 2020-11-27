#pragma once
class rnd_DescriptorHeapMgr
{
public:
   // Descriptor heaps
   ComPtr<ID3D12DescriptorHeap> dsvHeap;
   ComPtr<ID3D12DescriptorHeap> rtvHeap;
   ComPtr<ID3D12DescriptorHeap> cbvSrvUavHeap;
   ComPtr<ID3D12DescriptorHeap> svHeap; // sampler view

   std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> dsvHandle;
   std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> rtvHandle;
   std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> cbvSrvUavHandle;
   std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> samplerHandle;

   int dsvIncrSize;
   int rtvIncrSize;
   int cbvSrvUavIncrSize;
   int svIncrSize;

   int dsvCount = 10;
   int rtvCount = 10;
   int cbvSrvUavCount = 300;
   int svCount = 10;

   void OnInit(ID3D12Device* device);

   std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> GetDsvHandle();
   std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> GetRtvHandle();
   std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> GetCbvSrvUavHandle();
   std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> GetSamplerHandle();
};

