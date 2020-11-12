#pragma once
class siCommandList
{
   ComPtr<ID3D12GraphicsCommandList5> commandList;
public:
   ID3D12GraphicsCommandList5* operator->() const { return commandList.Get(); }
   [[nodiscard]] ID3D12GraphicsCommandList5* get() const { return commandList.Get(); }

   void onInit(ID3D12Device* device, ID3D12CommandAllocator* commandAllocator);
   void updateSubresource(ID3D12Resource* buffer, ID3D12Resource* uploadHeap, D3D12_SUBRESOURCE_DATA data) const;
};

