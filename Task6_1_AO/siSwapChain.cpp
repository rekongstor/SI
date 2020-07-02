#include "siSwapChain.h"
#include "siWindow.h"

void siSwapChain::onInit(siWindow* window, DXGI_SAMPLE_DESC sampleDesc, uint32_t bufferCount, IDXGIFactory4* factory, ID3D12CommandQueue* commandQueue)
{
   std::cout << "Initializing swap chain..." << std::endl;
   HRESULT hr = S_OK;
   DXGI_SWAP_CHAIN_DESC swapChainDesc = {
      {
         window->getWidth(),
         window->getHeight(),
         {0, 0},
         (DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM),
         (DXGI_MODE_SCANLINE_ORDER::DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED),
         (DXGI_MODE_SCALING::DXGI_MODE_SCALING_UNSPECIFIED)
      },
      sampleDesc,
      DXGI_USAGE_RENDER_TARGET_OUTPUT,
      (bufferCount),
      window->getWindow(),
      TRUE,
      (DXGI_SWAP_EFFECT_FLIP_DISCARD),
      0
   };
   IDXGISwapChain* tempSwapChain;
   hr = factory->CreateSwapChain(commandQueue, &swapChainDesc, &tempSwapChain);
   assert(hr == S_OK);

   swapChain = static_cast<IDXGISwapChain3*>(tempSwapChain);

   for (uint32_t i = 0; i < bufferCount; ++i)
   {
      hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
      assert(hr == S_OK);
   }
}
