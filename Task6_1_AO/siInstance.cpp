#include "siInstance.h"


void siInstance::initBuffer(ID3D12Device* device)
{
   HRESULT hr = device->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
      D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Buffer(data.size() * sizeof(perInstanceData)),
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr, IID_PPV_ARGS(&buffer));
   assert(hr == S_OK);
   hr = buffer->Map(0, nullptr, reinterpret_cast<void**>(&gpuAddress));
   assert(hr == S_OK);

   gpuVirtualAddress = buffer->GetGPUVirtualAddress();
}

void siInstance::gpuCopy()
{
   memcpy(gpuAddress, data.data(), data.size() * sizeof(perInstanceData));
}
