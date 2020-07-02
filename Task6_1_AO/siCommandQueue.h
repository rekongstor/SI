#pragma once
class siCommandQueue
{
   ComPtr<ID3D12CommandQueue> commandQueue;
public:
   ID3D12CommandQueue* operator->() const { return commandQueue.Get(); }
   ComPtr<ID3D12CommandQueue>& get() { return commandQueue; }

   void onInit(ID3D12Device* device);
};

