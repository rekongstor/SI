#pragma once
class siCommandAllocator
{
   uint32_t frameBufferCount;
   ComPtr<ID3D12CommandAllocator> commandAllocator[maxFrameBufferCount];
public:
   explicit siCommandAllocator(uint32_t frameBufferCount);
   ID3D12CommandAllocator* GetAllocator(uint32_t frame) const { return commandAllocator[frame].Get(); }

   void OnInit(ID3D12Device* device);
};
