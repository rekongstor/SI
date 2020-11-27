#pragma once
#include <memory>
#include <vector>


#include "rnd_Texture.h"

class rnd_CommandMgr
{
public:
   ComPtr<ID3D12CommandQueue> commandQueue;
   ComPtr<ID3D12CommandQueue> commandQueueCompute;
   ComPtr<ID3D12CommandQueue> commandQueueCopy;
   ComPtr<ID3D12CommandAllocator> commandAllocators[2];
   ComPtr<ID3D12CommandAllocator> commandAllocatorCompute;
   ComPtr<ID3D12CommandAllocator> commandAllocatorCopy;
   ComPtr<ID3D12GraphicsCommandList> commandList;
   ComPtr<ID3D12GraphicsCommandList> commandListCompute;
   ComPtr<ID3D12GraphicsCommandList> commandListCopy;

   ID3D12CommandList* Get() { return commandList.Get(); }

   void OnInit(ID3D12Device* device);
   void SetBarrier(const std::vector<std::pair<rnd_Texture&, D3D12_RESOURCE_STATES>>& texturesStates);
};

