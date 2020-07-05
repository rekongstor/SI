#pragma once
class siCommandList
{
   ComPtr<ID3D12GraphicsCommandList> commandList;
public:
   ID3D12GraphicsCommandList* operator->() const { return commandList.Get(); }
   [[nodiscard]] ID3D12GraphicsCommandList* get() const { return commandList.Get(); }

   void onInit(ID3D12Device* device, ID3D12CommandAllocator* commandAllocator);
   void updateSubresource(ID3D12Resource* buffer, ID3D12Resource* uploadHeap, D3D12_SUBRESOURCE_DATA textureData) const;
};

