#include "rnd_FenceMgr.h"

void rnd_FenceMgr::OnInit(ID3D12Device* device)
{
   ThrowIfFailed(device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
   ++fenceValue;
   fence->SetName(L"Fence");

   fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
   assert(fenceEvent != nullptr);
}

void rnd_FenceMgr::WaitForGpu(ID3D12CommandQueue* commandQueue)
{
   ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValue));

   ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
   ++fenceValue;

   WaitForSingleObject(fenceEvent, INFINITE);
}

void rnd_FenceMgr::MoveToNextFrame(ID3D12CommandQueue* commandQueue, IDXGISwapChain3* swapChain, int& currentFrame)
{
   fenceValues[currentFrame] = fenceValue;

   ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValue));
   ++fenceValue;

   currentFrame = swapChain->GetCurrentBackBufferIndex();

   if (fence->GetCompletedValue() < fenceValues[currentFrame])
   {
      ThrowIfFailed(fence->SetEventOnCompletion(fenceValues[currentFrame], fenceEvent));
      WaitForSingleObject(fenceEvent, INFINITE);
   }
}

rnd_FenceMgr::~rnd_FenceMgr()
{
   CloseHandle(fenceEvent);
}
