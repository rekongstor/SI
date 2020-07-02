#include "siRenderer.h"
#include "siWindow.h"

void siRenderer::UpdatePipeline()
{
}

siRenderer::siRenderer(siWindow* window, uint32_t bufferCount):
   window(window),
   bufferCount(bufferCount),
   commandAllocator(bufferCount),
   descriptorMgr(50, 50, 50, 50)
{
}

bool siRenderer::isActive() const
{
   return active;
}

void siRenderer::onInit()
{
   std::cout << "Initializing renderer..." << std::endl;
   HRESULT hr = S_OK;

   ComPtr<IDXGIFactory4> factory;
   hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
   assert(hr == S_OK);

   device.onInit(factory.Get(), featureLevel, softwareAdapter);
   commandQueue.onInit(device.get());
   commandAllocator.onInit(device.get());
   commandList.onInit(device.get(), commandAllocator.getAllocator(0));

   swapChain.onInit(window, sampleDesc, bufferCount, factory.Get(), commandQueue.get().Get());
   fenceMgr.onInit(device.get(), bufferCount, swapChain.get(), commandQueue.get());

   descriptorMgr.onInit(device.get());
   // [-bufferCount; -1] ids for swap chain buffers
   for (uint32_t i = 0; i < bufferCount; ++i)
   {
      auto& texture = textures[i - bufferCount];
      texture.initFromBuffer(swapChain.getBuffer(i));
      texture.createRtv(device.get(), &descriptorMgr);
   }
   // [0] id for depth/stencil buffer
   {
      auto& texture = textures[0];
      texture.initDepthStencil(device.get(), window->getWidth(), window->getHeight());
      texture.createDsv(device.get(), &descriptorMgr);
   }


   active = true;
}

void siRenderer::onUpdate()
{
   fenceMgr.waitForPreviousFrame(currentFrame);
   UpdatePipeline();

   ID3D12CommandList* ppCommandLists[] = { commandList.get() };
   commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);


}

void siRenderer::onDestroy()
{
}
