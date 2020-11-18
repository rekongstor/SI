#include "siCommandAllocator.h"

siCommandAllocator::siCommandAllocator(uint32_t frameBufferCount): frameBufferCount(frameBufferCount)
{
}

void siCommandAllocator::OnInit(ID3D12Device* device)
{
   std::cout << "Initializing command allocators..." << std::endl;
   HRESULT hr = S_OK;

   // Command Allocator
   for (uint32_t i = 0; i < frameBufferCount; ++i)
   {
      hr = device->CreateCommandAllocator(
         D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
         IID_PPV_ARGS(&commandAllocator[i]));
      assert(hr == S_OK);
      commandAllocator[i]->SetName(L"COMMAND ALLOCATOR");
   }
}
