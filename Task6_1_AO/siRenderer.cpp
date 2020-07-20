#include "siRenderer.h"
#include "siWindow.h"
#include "siImgui.h"
#include "siSceneLoader.h"
#include "siTimer.h"
#include <random>

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
   // initializing main objects
   {
      hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
      assert(hr == S_OK);

      device.onInit(factory.Get(), featureLevel, softwareAdapter);
      commandQueue.onInit(device.get());
      commandAllocator.onInit(device.get());
      commandList.onInit(device.get(), commandAllocator.getAllocator(0));

      swapChain.onInit(window, sampleDesc, bufferCount, factory.Get(), commandQueue.get().Get());
      fenceMgr.onInit(device.get(), bufferCount, swapChain.get(), commandQueue.get());

      descriptorMgr.onInit(device.get());
   }

   // imgui if exists
   if (imgui)
   {
      this->imgui = imgui;
      imgui->onInitRenderer(device.get(), bufferCount, descriptorMgr.getCbvSrvUavHeap().Get(),
                            descriptorMgr.getCbvSrvUavHandle());
      imgui->bindVariables(&camera.position, &camera.target, &targetOutput, &defRenderConstBuffer.get().lightColor,
                           &defRenderConstBuffer.get().ambientColor);
   }

   // swap chain buffers initialization
   for (uint32_t i = 0; i < bufferCount; ++i)
   {
      auto& texture = swapChainTargets[i];
      texture.initFromBuffer(swapChain.getBuffer(i), DXGI_FORMAT_UNKNOWN, sampleDesc);
      texture.createRtv(device.get(), &descriptorMgr);
      texture.setState(D3D12_RESOURCE_STATE_PRESENT);
   }

   // depth/stencil buffers
   {
      auto& depthStencilTarget = textures["#depthStencil"];
      depthStencilTarget.initDepthStencil(device.get(), window->getWidth(), window->getHeight());
      depthStencilTarget.createDsv(device.get(), &descriptorMgr);
   }

   // G-buffer
   {
      auto& diffuse = textures["#diffuseRenderTarget"];
      diffuse.initTexture(
         device.get(), window->getWidth(), window->getHeight(),
         DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON,
         sampleDesc);
      diffuse.createRtv(device.get(), &descriptorMgr);

      auto& positions = textures["#positionRenderTarget"];
      positions.initTexture(
         device.get(), window->getWidth(), window->getHeight(),
         DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON,
         sampleDesc);
      positions.createRtv(device.get(), &descriptorMgr);

      auto& normals = textures["#normalsRenderTarget"];
      normals.initTexture(
         device.get(), window->getWidth(), window->getHeight(),
         DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON,
         sampleDesc);
      normals.createRtv(device.get(), &descriptorMgr);
   }

   // SSAO
   {
      auto& texture = textures["#ssaoOutput"];
      texture.initTexture(
         device.get(), window->getWidth(), window->getHeight(),
         DXGI_FORMAT_R8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON,
         sampleDesc);
   }

   // SSAO blurred
   {
      auto& texture = textures["#ssaoOutputBlurred"];
      texture.initTexture(
         device.get(), window->getWidth(), window->getHeight(),
         DXGI_FORMAT_R8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON,
         sampleDesc);
   }

   // Deferred render target
   {
      auto& texture = textures["#deferredRenderTarget"];
      texture.initTexture(
         device.get(), window->getWidth(), window->getHeight(),
         DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON,
         sampleDesc);
   }

   // creating const buffers
   {
      mainConstBuffer.initBuffer({}, device.get());

      ssaoConstBuffer.initBuffer({}, device.get());
      defRenderConstBuffer.initBuffer({}, device.get());
   }

   // creating root signatures
   {
      auto& rs = rootSignatures["default"];
      rs.onInit(device.get(), siRootSignature::createSampleRsBlob()); // blobs should be loaded from files
   }

   // creating PSOs
   {
      auto& pso = pipelineStates["default"];
      DXGI_FORMAT rtvFormats[] = {
         DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT
      };
      pso.createPso(device.get(), rootSignatures["default"].get(), L"defRenderVS.hlsl", L"defRenderPS.hlsl",
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

   // creating compute shaders
   {
      auto& ssao = computeShaders["ssao"];
      ssao.onInit(device.get(), &descriptorMgr, L"ssao.hlsl",
                  {textures["#depthStencil"], textures["#normalsRenderTarget"], textures["#positionRenderTarget"]},
                  {textures["#ssaoOutput"]},
                  ssaoConstBuffer.getGpuVirtualAddress());

      auto& ssaoBlurred = computeShaders["ssaoBlur"];
      ssaoBlurred.onInit(device.get(), &descriptorMgr, L"ssaoBlur.hlsl",
         { textures["#ssaoOutput"]},
         { textures["#ssaoOutputBlurred"] },
         ssaoConstBuffer.getGpuVirtualAddress());

      auto& deferredRender = computeShaders["deferredRender"];
      deferredRender.onInit(device.get(), &descriptorMgr, L"pbrRender.hlsl",
                            {
                               textures["#diffuseRenderTarget"], textures["#positionRenderTarget"],
                               textures["#normalsRenderTarget"], textures["#ssaoOutputBlurred"]
                            },
                            {textures["#deferredRenderTarget"]},
                            defRenderConstBuffer.getGpuVirtualAddress());
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
            XMStoreFloat4x4(&instanceData.world, world);
            XMStoreFloat4x4(&instanceData.worldIt, InverseTranspose(world));

            inst.get().emplace_back(instanceData);
         }
      }
      inst.initBuffer(device.get());
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
   auto& mainCb = mainConstBuffer.get();
   XMStoreFloat4x4(&mainCb.viewMatrix, camera.viewMatrix);
   XMStoreFloat4x4(&mainCb.projMatrix, camera.projMatrix);
   mainConstBuffer.gpuCopy();

   auto& ssaoCb = ssaoConstBuffer.get();
   XMStoreFloat4x4(&ssaoCb.projMatrix, camera.projMatrix);
   ssaoCb.width = window->getWidth();
   ssaoCb.height = window->getHeight();
   ssaoConstBuffer.gpuCopy();

   auto& defRenCb = defRenderConstBuffer.get();
   float4 lightDirection = {0.f, -1.f, 0.f, 0.f};
   float4 lightColor = { 0.f, 0.f, 0.f, 1.f };
   float4 ambientColor = { 1.1f, 1.1f, 1.1f, 1.f };
   XMStoreFloat4(&defRenCb.lightDirection,
                 XMVector4Transform(XMLoadFloat4(&lightDirection), XMMatrixTranspose(camera.viewMatrix)));
   defRenCb.targetOutput = targetOutput;
   defRenCb.lightColor = lightColor;
   defRenCb.ambientColor = ambientColor;
   defRenderConstBuffer.gpuCopy();


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

   const float clearColor[] = {0.0f, 0.0f, 0.0f, 1.0f};
   auto& swapChainTarget = swapChainTargets[currentFrame];
   auto& depthStencil = textures["#depthStencil"];

   auto& positionRenderTarget = textures["#positionRenderTarget"];
   auto& normalsRenderTarget = textures["#normalsRenderTarget"];
   auto& diffuseRenderTarget = textures["#diffuseRenderTarget"];

   auto& deferredRenderTarget = textures["#deferredRenderTarget"];

   // drawing
   {
      diffuseRenderTarget.resourceBarrier(commandList.get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
      positionRenderTarget.resourceBarrier(commandList.get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
      normalsRenderTarget.resourceBarrier(commandList.get(), D3D12_RESOURCE_STATE_RENDER_TARGET);

      D3D12_CPU_DESCRIPTOR_HANDLE rts[] = {
         diffuseRenderTarget.getRtvHandle().first,
         positionRenderTarget.getRtvHandle().first,
         normalsRenderTarget.getRtvHandle().first
      };
      commandList->OMSetRenderTargets(_countof(rts),
                                      rts,
                                      FALSE,
                                      &depthStencil.getDsvHandle().first);


      commandList->ClearRenderTargetView(diffuseRenderTarget.getRtvHandle().first, clearColor, 0, nullptr);
      commandList->ClearRenderTargetView(positionRenderTarget.getRtvHandle().first, clearColor, 0, nullptr);
      commandList->ClearRenderTargetView(normalsRenderTarget.getRtvHandle().first, clearColor, 0, nullptr);
      commandList->ClearDepthStencilView(depthStencil.getDsvHandle().first, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

      commandList->RSSetViewports(1, &viewportScissor.getViewport());
      commandList->RSSetScissorRects(1, &viewportScissor.getScissorRect());

      // Root signature [0]. Drawing meshes
      commandList->SetGraphicsRootSignature(rootSignatures["default"].get().Get());
      commandList->SetGraphicsRootConstantBufferView(0, mainConstBuffer.getGpuVirtualAddress());

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
         commandList->DrawIndexedInstanced(mesh.getIndexCount(), static_cast<UINT>(instance.second.get().size()), 0, 0,
                                           0);
      }
      diffuseRenderTarget.resourceBarrier(commandList.get(), D3D12_RESOURCE_STATE_COMMON);
      positionRenderTarget.resourceBarrier(commandList.get(), D3D12_RESOURCE_STATE_COMMON);
      normalsRenderTarget.resourceBarrier(commandList.get(), D3D12_RESOURCE_STATE_COMMON);
   }

   // compute shaders
   {
      computeShaders["ssao"].dispatch(commandList.get(), window->getWidth(), window->getHeight());
      computeShaders["ssaoBlur"].dispatch(commandList.get(), window->getWidth(), window->getHeight());
      computeShaders["deferredRender"].dispatch(commandList.get(), window->getWidth(), window->getHeight());
   }

   // Copying compute shader results to render target
   {
      swapChainTarget.resourceBarrier(commandList.get(), D3D12_RESOURCE_STATE_COPY_DEST);
      deferredRenderTarget.resourceBarrier(commandList.get(), D3D12_RESOURCE_STATE_COPY_SOURCE);
      commandList->CopyResource(swapChainTarget.getBuffer().Get(), deferredRenderTarget.getBuffer().Get());
      swapChainTarget.resourceBarrier(commandList.get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
      deferredRenderTarget.resourceBarrier(commandList.get(), D3D12_RESOURCE_STATE_COMMON);
   }

   // imgui draw
   {
      commandList->OMSetRenderTargets(1,
                                      &swapChainTarget.getRtvHandle().first,
                                      FALSE,
                                      &depthStencil.getDsvHandle().first);
      if (imgui)
         imgui->onRender(commandList.get());
   }

   swapChainTarget.resourceBarrier(commandList.get(), D3D12_RESOURCE_STATE_PRESENT);
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
