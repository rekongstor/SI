#include "siCommandList.h"

void siCommandList::onInit(ID3D12Device* device, ID3D12CommandAllocator* commandAllocator)
{
   std::cout << "Initializing command list..." << std::endl;
   HRESULT hr = S_OK;

   hr = device->CreateCommandList(
      0,
      D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
      commandAllocator,
      nullptr,
      IID_PPV_ARGS(&commandList)
   );
   assert(hr == S_OK);
}
