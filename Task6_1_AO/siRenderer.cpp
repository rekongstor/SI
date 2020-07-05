#include "siRenderer.h"
#include "siWindow.h"
#include "siImgui.h"
#include "siSceneLoader.h"

void siRenderer::UpdatePipeline()
{
   HRESULT hr = S_OK;

   hr = commandAllocator.getAllocator(currentFrame)->Reset();
   assert(hr == S_OK);


   hr = commandList->Reset(commandAllocator.getAllocator(currentFrame),
                           pipelineStates[0].getPipelineState().Get());
   assert(hr == S_OK);

   auto& renderTarget = swapChainTargets[currentFrame];
   auto& depthStencil = depthStencilTarget;
   commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
                                   renderTarget.getBuffer().Get(),
                                   D3D12_RESOURCE_STATE_PRESENT,
                                   D3D12_RESOURCE_STATE_RENDER_TARGET));


   commandList->OMSetRenderTargets(1,
                                   &renderTarget.getRtvHandle().first,
                                   FALSE,
                                   &textures[0].getDsvHandle().first);


   const float clearColor[] = {0.0f, 0.2f, 0.4f, 1.0f};
   commandList->ClearRenderTargetView(renderTarget.getRtvHandle().first, clearColor, 0, nullptr);
   commandList->ClearDepthStencilView(depthStencil.getDsvHandle().first, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

   commandList->RSSetViewports(1, &viewportScissor.getViewport());
   commandList->RSSetScissorRects(1, &viewportScissor.getScissorRect());

   ID3D12DescriptorHeap* d[] = {descriptorMgr.getCbvSrvUavHeap().Get()};
   commandList->SetDescriptorHeaps(1, d);
   if (imgui)
      imgui->onRender(commandList.get());

   commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
                                   renderTarget.getBuffer().Get(),
                                   D3D12_RESOURCE_STATE_RENDER_TARGET,
                                   D3D12_RESOURCE_STATE_PRESENT));


   hr = commandList->Close();
   assert(hr == S_OK);
}

siRenderer::siRenderer(siWindow* window, uint32_t bufferCount):
   window(window),
   bufferCount(bufferCount),
   commandAllocator(bufferCount),
   descriptorMgr(50, 50, 50, 50),
   viewportScissor(window->getWidth(), window->getHeight())
{
}

bool siRenderer::isActive() const
{
   return active;
}

void siRenderer::onInit(siImgui* imgui)
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

   // imgui if exists
   if (imgui)
   {
      this->imgui = imgui;
      imgui->onInitRenderer(device.get(), bufferCount, descriptorMgr.getCbvSrvUavHeap().Get(),
                            descriptorMgr.getCbvSrvUavHandle());
   }

   // swap chain buffers initialization
   for (uint32_t i = 0; i < bufferCount; ++i)
   {
      auto& texture = swapChainTargets[i];
      texture.initFromBuffer(swapChain.getBuffer(i));
      texture.createRtv(device.get(), &descriptorMgr);
   }

   depthStencilTarget.initDepthStencil(device.get(), window->getWidth(), window->getHeight());
   depthStencilTarget.createDsv(device.get(), &descriptorMgr);

   // creating root signatures
   {
      auto& rs = rootSignatures[0];
      rs.onInit(device.get(), rs.createSampleRsBlob()); // blobs should be loaded from files
   }
   // creating PSOs
   {
      auto& pso = pipelineStates[0];
      pso.createPso(device.get(), rootSignatures[0].get(), L"pbrRenderVS.hlsl", L"pbrRenderPS.hlsl",
                    DXGI_FORMAT_R8G8B8A8_UNORM, sampleDesc,
                    {
                       {
                          "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0
                       },
                       {
                          "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,D3D12_APPEND_ALIGNED_ELEMENT
                       },
                       {
                          "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,D3D12_APPEND_ALIGNED_ELEMENT
                       }
                    }
      );
   }

   siSceneLoader::loadScene("sponza.obj", meshes, textures, device.get(), commandList, &descriptorMgr);


   active = true;
}


void siRenderer::onUpdate()
{
   if (imgui)
      imgui->onUpdate();

   fenceMgr.waitForPreviousFrame(currentFrame);
   UpdatePipeline();

   ID3D12CommandList* ppCommandLists[] = {commandList.get()};
   commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

   fenceMgr.signalCommandQueue(currentFrame);

   HRESULT hr = swapChain->Present(0, 0);
   assert(hr == S_OK);
}

void siRenderer::onDestroy()
{
}
