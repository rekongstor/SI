#include "siFenceMgr.h"

void siFenceMgr::onInit(ID3D12Device* device, uint32_t bufferCount, const ComPtr<IDXGISwapChain3>& swapChain, const ComPtr<ID3D12CommandQueue>& cmdQueue)
{
   std::cout << "Initializing fences..." << std::endl;
   HRESULT hr = S_OK;

   for (uint32_t i = 0; i < bufferCount; ++i)
   {
      hr = device->CreateFence(
         NULL,
         D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE,
         IID_PPV_ARGS(&fences[i])
      );
      assert(hr == S_OK);
      fenceValues[i] = 0;
   }

   fenceEvent = CreateEvent(
      nullptr,
      FALSE,
      FALSE,
      nullptr
   );
   hr = fenceEvent ? S_OK : E_FAIL;
   assert(hr == S_OK);

   this->swapChain = swapChain;
   this->commandQueue = cmdQueue;
}

void siFenceMgr::waitForPreviousFrame(uint32_t& currentFrame)
{
   HRESULT hr;

   currentFrame = swapChain->GetCurrentBackBufferIndex();
   if (fences[currentFrame]->GetCompletedValue() < fenceValues[currentFrame])
   {
      hr = fences[currentFrame]->SetEventOnCompletion(fenceValues[currentFrame], fenceEvent);
      assert(hr == S_OK);
      WaitForSingleObject(fenceEvent, INFINITE);
   }

   ++fenceValues[currentFrame];
}

void siFenceMgr::signalCommandQueue(uint32_t currentFrame)
{
   HRESULT hr = commandQueue->Signal(fences[currentFrame].Get(), fenceValues[currentFrame]);
   assert(hr == S_OK);
}

siFenceMgr::~siFenceMgr()
{
   CloseHandle(fenceEvent);
}
