#pragma once
class rnd_FenceMgr
{
public:
   void OnInit(ID3D12Device* device);
   void WaitForGpu(ID3D12CommandQueue* commandQueue);
   void MoveToNextFrame(ID3D12CommandQueue* commandQueue, IDXGISwapChain3* swapChain, int& currentFrame);
   ~rnd_FenceMgr();

   ComPtr<ID3D12Fence1> fence;
   uint64_t fenceValues[2];
   uint64_t fenceValue = 0;
   HANDLE fenceEvent;

   // TODO: Create fences for Compute/Copy command buffers
};

