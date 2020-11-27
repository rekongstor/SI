#include "rnd_CommandMgr.h"
#include "rnd_Texture.h"

void rnd_CommandMgr::OnInit(ID3D12Device* device)
{
   // TODO: Add bundles?
#pragma region CommandQueues 
   D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {
      D3D12_COMMAND_LIST_TYPE_DIRECT,
      D3D12_COMMAND_QUEUE_PRIORITY_NORMAL, // set higher priority, could be failed
      D3D12_COMMAND_QUEUE_FLAG_NONE, // disable GPU timeout
      0 // multi-GPU
   };
   ThrowIfFailed(device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue)));
   commandQueue->SetName(L"Command queue direct");
   commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
   ThrowIfFailed(device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueueCompute)));
   commandQueueCompute->SetName(L"Command queue compute");
   commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
   ThrowIfFailed(device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueueCopy)));
   commandQueueCopy->SetName(L"Command queue copy");

#pragma endregion 

#pragma region CommandAllocators
   ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[0])));
   commandAllocators[0]->SetName(L"COMMAND ALLOCATOR 0: Direct");
   ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[1])));
   commandAllocators[1]->SetName(L"COMMAND ALLOCATOR 1: Direct");

   ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&commandAllocatorCompute)));
   commandAllocatorCompute->SetName(L"COMMAND ALLOCATOR: Compute");
   ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&commandAllocatorCopy)));
   commandAllocatorCopy->SetName(L"COMMAND ALLOCATOR: Copy");
#pragma endregion 

#pragma region CommandList
   ThrowIfFailed(device->CreateCommandList(        /*multi-gpu*/ 0,
      D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[0].Get(), /* Initial PSO */ nullptr,
      IID_PPV_ARGS(&commandList)));
   commandList->SetName(L"Command List Direct");
   commandList->Close();

   ThrowIfFailed(device->CreateCommandList(        /*multi-gpu*/ 0,
      D3D12_COMMAND_LIST_TYPE_COMPUTE, commandAllocatorCompute.Get(), /* Initial PSO */ nullptr,
      IID_PPV_ARGS(&commandListCompute)));
   commandListCompute->SetName(L"Command List Compute");
   commandListCompute->Close();

   ThrowIfFailed(device->CreateCommandList(        /*multi-gpu*/ 0,
      D3D12_COMMAND_LIST_TYPE_COPY, commandAllocatorCopy.Get(), /* Initial PSO */ nullptr,
      IID_PPV_ARGS(&commandListCopy)));
   commandListCopy->SetName(L"Command List Copy");
   commandListCopy->Close();
#pragma endregion 
}

void rnd_CommandMgr::SetBarrier(const std::vector<std::pair<rnd_Texture&, D3D12_RESOURCE_STATES>>& texturesStates)
{
   std::vector<D3D12_RESOURCE_BARRIER> barriers;
   for (auto& texSt : texturesStates)
   {
      if (texSt.first.state != texSt.second)
      {
         barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(texSt.first.buffer.Get(), texSt.first.state, texSt.second));
         texSt.first.SetState(texSt.second);
      }
   }
   if (!barriers.empty()) {
      commandList->ResourceBarrier(barriers.size(), barriers.data());
   }
}
