#pragma once
class siCommandList
{
   ComPtr<ID3D12GraphicsCommandList> commandList;
public:
   ID3D12GraphicsCommandList* operator->() const { return commandList.Get(); }
   [[nodiscard]] ID3D12GraphicsCommandList* get() const { return commandList.Get(); }

   void onInit(ID3D12Device* device, ID3D12CommandAllocator* commandAllocator);
};

