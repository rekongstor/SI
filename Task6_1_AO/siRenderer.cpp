#include "siRenderer.h"
#include "siWindow.h"
#include "siImgui.h"
#include "siSceneLoader.h"
#include "siTimer.h"

siRenderer::siRenderer(siWindow* window, uint32_t bufferCount):
   window(window),
   bufferCount(bufferCount),
   commandAllocator(bufferCount),
   descriptorMgr(50, 50, 50, 50),
   viewportScissor(window->getWidth(), window->getHeight()),
   camera({13.f, 4.f, 9.f, 1.f}, {0.f, 0.f, 0.f, 1.f}, 75.f,
          static_cast<float>(window->getWidth()) / static_cast<float>(window->getHeight()))
{
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
      imgui->bindVariables(&camera.position, &camera.target);
   }

   // swap chain buffers initialization
   for (uint32_t i = 0; i < bufferCount; ++i)
   {
      auto& texture = swapChainTargets[i];
      texture.initFromBuffer(swapChain.getBuffer(i), DXGI_FORMAT_UNKNOWN, sampleDesc);
      texture.createRtv(device.get(), &descriptorMgr);
   }

   // depth/stencil buffers
   {
      auto& depthStencilTarget = textures["#depthStencil"];
      depthStencilTarget.initDepthStencil(device.get(), window->getWidth(), window->getHeight());
      depthStencilTarget.createDsv(device.get(), &descriptorMgr);
      depthStencilTarget.createSrv(device.get(), &descriptorMgr);
   }

   // normals render target buffer
   {
      auto& normalsRenderTarget = textures["#normalsRenderTarget"];
      normalsRenderTarget.initTexture(
         device.get(), window->getWidth(), window->getHeight(),
         DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE, sampleDesc);
      normalsRenderTarget.createRtv(device.get(), &descriptorMgr);

      normalsRenderTarget.createSrv(device.get(), &descriptorMgr);
   }

   // creating root signatures
   {
      auto& rs = rootSignatures["default"];
      rs.onInit(device.get(), siRootSignature::createSampleRsBlob()); // blobs should be loaded from files
   }
   {
      auto& rsCs = rootSignatures["csCb1In1Out"];
      rsCs.onInit(device.get(), siRootSignature::createCsRsBlobCb1In1Out());
   }

   // creating PSOs
   {
      auto& pso = pipelineStates["default"];
      DXGI_FORMAT rtvFormats[] = {DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM};
      pso.createPso(device.get(), rootSignatures["default"].get(), L"pbrRenderVS.hlsl", L"pbrRenderPS.hlsl",
                    rtvFormats, _countof(rtvFormats),
                    sampleDesc, {
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
   {
      auto& pso = pipelineStates["depthPrepare"];
      pso.createPso(device.get(), rootSignatures["csCb1In1Out"].get(), L"depthPrepare.hlsl");
   }

   siSceneLoader::loadScene("monkey.obj", meshes, textures, device.get(), commandList, &descriptorMgr);

   // creating instances for [0] mesh
   {
      auto& inst = instances[0];
      const uint32_t x = 3;
      const uint32_t y = 3;
      float scale = 1.f;
      XMFLOAT4 position = XMFLOAT4();
      XMFLOAT4 rotation = XMFLOAT4();
      for (uint32_t i = 0; i < x; ++i)
      {
         for (uint32_t j = 0; j < y; ++j)
         {
            XMStoreFloat4(&rotation, XMQuaternionRotationRollPitchYaw(
                             0.f,
                             0.f,
                             0.f
                          ));
            position = {
               (static_cast<float>(i) - static_cast<float>(x - 1) * 0.5f) * (scale * 3.5f + 1.f), 0.f,
               (static_cast<float>(j) - static_cast<float>(y - 1) * 0.5f) * (scale * 3.5f + 1.f), 1.f
            };
            XMVECTOR rotVector(XMLoadFloat4(&rotation));
            XMMATRIX rotMatrix = XMMatrixRotationQuaternion(rotVector);
            XMVECTOR transVector(XMLoadFloat4(&position));
            XMMATRIX transMatrix = XMMatrixTranslationFromVector(transVector);
            XMMATRIX scaleMatrix = XMMatrixScaling(scale, scale, scale);
            auto InverseTranspose = [](CXMMATRIX M)
            {
               XMMATRIX A = M;
               A.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
               XMVECTOR det = XMMatrixDeterminant(A);
               return XMMatrixTranspose(XMMatrixInverse(&det,
                                                        A));
            };
            XMMATRIX world = scaleMatrix * rotMatrix * transMatrix;
            perInstanceData instanceData;
            XMStoreFloat4x4(&instanceData.world, XMMatrixTranspose(world));
            XMStoreFloat4x4(&instanceData.worldIt, XMMatrixTranspose(InverseTranspose(world)));

            inst.get().emplace_back(instanceData);
         }
      }
      inst.initBuffer(device.get());
   }

   for (uint32_t i = 0; i < bufferCount; ++i)
   {
      mainConstBuffer[i].initBuffer(
         {
            XMFLOAT4X4(),
            XMFLOAT4(),
            {0.f, -1.f, 0.f, 0.f},
            {5.f, 5.f, 5.f, 1.f},
            {.1f, .1f, .1f, 1.f}
         }, device.get());
   }

   executePipeline();
   active = true;
}

void siRenderer::update()
{
   static siTimer timer;
   static float timeLeft = 0.f;
   float delta = timer.delta();
   timeLeft += delta * 0.000001f;

   camera.update();
   auto& cb = mainConstBuffer[currentFrame].get();
   cb.camPos = camera.position;
   cb.vpMatrix = camera.vpMatrix;
   mainConstBuffer[currentFrame].gpuCopy();

   for (auto& inst : instances)
   {
      inst.second.gpuCopy();
   }
}

void siRenderer::updatePipeline()
{
   HRESULT hr = S_OK;

   hr = commandAllocator.getAllocator(currentFrame)->Reset();
   assert(hr == S_OK);


   hr = commandList->Reset(commandAllocator.getAllocator(currentFrame),
                           pipelineStates["default"].getPipelineState().Get());
   assert(hr == S_OK);

   auto& renderTarget = swapChainTargets[currentFrame];
   auto& normalsRenderTarget = textures["#normalsRenderTarget"];
   auto& depthStencil = textures["#depthStencil"];
   commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
                                   renderTarget.getBuffer().Get(),
                                   D3D12_RESOURCE_STATE_PRESENT,
                                   D3D12_RESOURCE_STATE_RENDER_TARGET));
   commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
                                   normalsRenderTarget.getBuffer().Get(),
                                   D3D12_RESOURCE_STATE_COPY_SOURCE,
                                   D3D12_RESOURCE_STATE_RENDER_TARGET));

   D3D12_CPU_DESCRIPTOR_HANDLE rts[] = {renderTarget.getRtvHandle().first, normalsRenderTarget.getRtvHandle().first};
   commandList->OMSetRenderTargets(2,
                                   rts,
                                   FALSE,
                                   &depthStencil.getDsvHandle().first);


   const float clearColor[] = {0.0f, 0.0f, 0.0f, 1.0f};
   commandList->ClearRenderTargetView(renderTarget.getRtvHandle().first, clearColor, 0, nullptr);
   commandList->ClearRenderTargetView(normalsRenderTarget.getRtvHandle().first, clearColor, 0, nullptr);
   commandList->ClearDepthStencilView(depthStencil.getDsvHandle().first, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

   commandList->RSSetViewports(1, &viewportScissor.getViewport());
   commandList->RSSetScissorRects(1, &viewportScissor.getScissorRect());

   // Root signature [0]. Drawing meshes
   commandList->SetGraphicsRootSignature(rootSignatures["default"].get().Get());
   commandList->SetGraphicsRootConstantBufferView(0, mainConstBuffer[currentFrame].getGpuVirtualAddress());

   commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

   for (auto& instance : instances)
   {
      auto it = meshes.find(instance.first);
      if (it == meshes.end())
         continue;

      auto& mesh = it->second;
      commandList->IASetVertexBuffers(0, 1, &mesh.getVertexBufferView());
      commandList->IASetIndexBuffer(&mesh.getIndexBufferView());
      commandList->SetGraphicsRootShaderResourceView(1, instance.second.getGpuVirtualAddress(currentFrame));
      commandList->SetGraphicsRootDescriptorTable(2, textures[mesh.getDiffuseMap()].getSrvHandle().second);
      commandList->DrawIndexedInstanced(mesh.getIndexCount(), static_cast<UINT>(instance.second.get().size()), 0, 0, 0);
   }

   // Copying compute shader results to render target
   {
      commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
                                      normalsRenderTarget.getBuffer().Get(),
                                      D3D12_RESOURCE_STATE_RENDER_TARGET,
                                      D3D12_RESOURCE_STATE_COPY_SOURCE));
      commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
                                      renderTarget.getBuffer().Get(),
                                      D3D12_RESOURCE_STATE_RENDER_TARGET,
                                      D3D12_RESOURCE_STATE_COPY_DEST));
      commandList->CopyResource(renderTarget.getBuffer().Get(), normalsRenderTarget.getBuffer().Get());
      commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
                                      renderTarget.getBuffer().Get(),
                                      D3D12_RESOURCE_STATE_COPY_DEST,
                                      D3D12_RESOURCE_STATE_RENDER_TARGET));
   }


   if (imgui)
      imgui->onRender(commandList.get());

   commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
                                   renderTarget.getBuffer().Get(),
                                   D3D12_RESOURCE_STATE_RENDER_TARGET,
                                   D3D12_RESOURCE_STATE_PRESENT));
}

void siRenderer::executePipeline()
{
   HRESULT hr = commandList->Close();
   assert(hr == S_OK);

   ID3D12CommandList* ppCommandLists[] = {commandList.get()};
   commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

   fenceMgr.signalCommandQueue(currentFrame);
}

void siRenderer::onUpdate()
{
   fenceMgr.waitForPreviousFrame(currentFrame);

   update();
   if (imgui)
      imgui->onUpdate();

   updatePipeline();
   executePipeline();

   HRESULT hr = swapChain->Present(0, 0);
   assert(hr == S_OK);
}

bool siRenderer::isActive() const
{
   return active;
}
