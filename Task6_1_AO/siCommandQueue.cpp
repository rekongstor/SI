#include "siCommandQueue.h"

void siCommandQueue::onInit(ID3D12Device* device)
{
   std::cout << "Initializing command queue..." << std::endl;
   HRESULT hr = S_OK;

   D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {
      D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
      D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
      D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE,
      0
   };
   hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));
   assert(hr == S_OK);
}
