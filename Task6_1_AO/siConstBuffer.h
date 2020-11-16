#pragma once
#include "alignment.h"

#define CONST_BUFFER_SIZE(Data)  AlignSize(Data, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

class siDescriptorMgr;


template <class T>
class siConstBuffer
{
   T data;
   uint8_t* gpuAddress = nullptr;
   ComPtr<ID3D12Resource> buffer;
   D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress;
public:
   T& Get() { return data; }
   [[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const { return gpuVirtualAddress; }

   void InitBuffer(T data, ID3D12Device* device);
   void GpuCopy();
};

template <class T>
void siConstBuffer<T>::InitBuffer(T data, ID3D12Device* device)
{
   this->data = data;

   HRESULT hr = device->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
      D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Buffer(AlignSize(sizeof(data), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)),
      D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&buffer)
   );
   assert(hr == S_OK);

   hr = buffer->Map(0, nullptr, reinterpret_cast<void**>(&gpuAddress));
   assert(hr == S_OK);

   gpuVirtualAddress = buffer->GetGPUVirtualAddress();
}

template <class T>
void siConstBuffer<T>::GpuCopy()
{
   memcpy(gpuAddress, &data, sizeof(T));
}