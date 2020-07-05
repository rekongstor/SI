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

void siCommandList::updateSubresource(ID3D12Resource* buffer, ID3D12Resource* uploadHeap, D3D12_SUBRESOURCE_DATA textureData) const
{
   UpdateSubresources(commandList.Get(), buffer, uploadHeap, 0, 0, 1, &textureData);

   commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
      buffer, D3D12_RESOURCE_STATE_COPY_DEST,
      D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
}
