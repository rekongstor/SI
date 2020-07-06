#pragma once
class siDescriptorMgr;

template <class T>
constexpr UINT64 cbSize()
{
   return ((sizeof(T) + 255) & ~255) >> 8;
}


struct mainConstBuff
{
   XMFLOAT4X4 vpMatrix;
   XMFLOAT4 camPos;
   XMFLOAT4 lightDirection;
   XMFLOAT4 lightColor;
   XMFLOAT4 ambientColor;
};

template <class T>
class siConstBuffer
{
   T data;
   uint8_t* gpuAddress = nullptr;
   ComPtr<ID3D12Resource> buffer;
public:
   T& get() { return data; }
   void initBuffer(const T& data, ID3D12Device* device);
   void gpuCopy();
};


template <class T>
void siConstBuffer<T>::initBuffer(const T& data, ID3D12Device* device)
{
   this->data = data;
   HRESULT hr = device->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
      D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Buffer(1024 * 64 * cbSize<T>()),
      D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&buffer)
   );
   assert(hr == S_OK);

   hr = buffer->Map(0, nullptr, reinterpret_cast<void**>(&gpuAddress));
   assert(hr == S_OK);
}

template <class T>
void siConstBuffer<T>::gpuCopy()
{
   memcpy(gpuAddress, &data, sizeof(T));
}
