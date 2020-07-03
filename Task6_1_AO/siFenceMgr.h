#pragma once
class siFenceMgr
{
   ComPtr<ID3D12Fence> fences[maxFrameBufferCount];
   uint64_t fenceValues[maxFrameBufferCount];
   HANDLE fenceEvent;
   ComPtr<IDXGISwapChain3> swapChain;
   ComPtr<ID3D12CommandQueue> commandQueue;
public:
   void onInit(ID3D12Device* device, uint32_t bufferCount, const ComPtr<IDXGISwapChain3>& swapChain, const ComPtr<ID3D12CommandQueue>& cmdQueue);
   void waitForPreviousFrame(uint32_t& currentFrame);
   void signalCommandQueue(uint32_t currentFrame);

   ~siFenceMgr();
};

