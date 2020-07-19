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
      //texture.createSrv(device.get(), &descriptorMgr);
   }

   // depth/stencil buffers
   {
      auto& depthStencilTarget = textures["#depthStencil"];
      depthStencilTarget.initDepthStencil(device.get(), window->getWidth(), window->getHeight());
      depthStencilTarget.createDsv(device.get(), &descriptorMgr);
   }

   // cacao post-process buffers
   {
      auto& normalsRenderTarget = textures["#normalsRenderTarget"];
      normalsRenderTarget.initTexture(
         device.get(), window->getWidth(), window->getHeight(),
         DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE,
         sampleDesc);
      normalsRenderTarget.createRtv(device.get(), &descriptorMgr);
   }
   {
      auto& texture = textures["#depthPrepared"];
      texture.initTexture(
         device.get(), window->getWidth(), window->getHeight(),
         DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE,
         sampleDesc);
   }

   // creating const buffers
   {
      mainConstBuffer.initBuffer(
         {
            XMFLOAT4X4(),
            XMFLOAT4(),
            {0.f, -1.f, 0.f, 0.f},
            {5.f, 5.f, 5.f, 1.f},
            {.1f, .1f, .1f, 1.f}
         }, device.get());

      csConstBuffer.initBuffer(
         {
            XMFLOAT4X4()
         }, device.get());
   }

   // creating root signatures
   {
      auto& rs = rootSignatures["default"];
      rs.onInit(device.get(), siRootSignature::createSampleRsBlob()); // blobs should be loaded from files
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

   // creating compute shaders
   {
      auto& cs = computeShaders["depthPrepare"];
      cs.onInit(device.get(), &descriptorMgr, L"depthPrepare.hlsl",
                {textures["#depthStencil"]}, { textures["#depthPrepared"] }, csConstBuffer.getGpuVirtualAddress());
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


   executePipeline();
   active = true;
}


static void updateConstants(FfxCacaoConstants* consts, FfxCacaoSettings* settings, BufferSizeInfo* bufferSizeInfo,
                            const FfxCacaoMatrix4x4* proj, const FfxCacaoMatrix4x4* normalsToView)
{
   consts->BilateralSigmaSquared = settings->bilateralSigmaSquared;
   consts->BilateralSimilarityDistanceSigma = settings->bilateralSimilarityDistanceSigma;

   // used to get average load per pixel; 9.0 is there to compensate for only doing every 9th InterlockedAdd in PSPostprocessImportanceMapB for performance reasons
   consts->LoadCounterAvgDiv = 9.0f / (float)(bufferSizeInfo->importanceMapWidth * bufferSizeInfo->importanceMapHeight *
      255.0);

   float depthLinearizeMul = (MATRIX_ROW_MAJOR_ORDER) ? (-proj->elements[3][2]) : (-proj->elements[2][3]);
   // float depthLinearizeMul = ( clipFar * clipNear ) / ( clipFar - clipNear );
   float depthLinearizeAdd = (MATRIX_ROW_MAJOR_ORDER) ? (proj->elements[2][2]) : (proj->elements[2][2]);
   // float depthLinearizeAdd = clipFar / ( clipFar - clipNear );
   // correct the handedness issue. need to make sure this below is correct, but I think it is.
   if (depthLinearizeMul * depthLinearizeAdd < 0)
      depthLinearizeAdd = -depthLinearizeAdd;
   consts->DepthUnpackConsts[0] = depthLinearizeMul;
   consts->DepthUnpackConsts[1] = depthLinearizeAdd;

   float tanHalfFOVY = 1.0f / proj->elements[1][1]; // = tanf( drawContext.Camera.GetYFOV( ) * 0.5f );
   float tanHalfFOVX = 1.0F / proj->elements[0][0]; // = tanHalfFOVY * drawContext.Camera.GetAspect( );
   consts->CameraTanHalfFOV[0] = tanHalfFOVX;
   consts->CameraTanHalfFOV[1] = tanHalfFOVY;

   consts->NDCToViewMul[0] = consts->CameraTanHalfFOV[0] * 2.0f;
   consts->NDCToViewMul[1] = consts->CameraTanHalfFOV[1] * -2.0f;
   consts->NDCToViewAdd[0] = consts->CameraTanHalfFOV[0] * -1.0f;
   consts->NDCToViewAdd[1] = consts->CameraTanHalfFOV[1] * 1.0f;

   float ratio = ((float)bufferSizeInfo->inputOutputBufferWidth) / ((float)bufferSizeInfo->depthBufferWidth);
   float border = (1.0f - ratio) / 2.0f;
   for (int i = 0; i < 2; ++i)
   {
      consts->DepthBufferUVToViewMul[i] = consts->NDCToViewMul[i] / ratio;
      consts->DepthBufferUVToViewAdd[i] = consts->NDCToViewAdd[i] - consts->NDCToViewMul[i] * border / ratio;
   }

   consts->EffectRadius = FFX_CACAO_CLAMP(settings->radius, 0.0f, 100000.0f);
   consts->EffectShadowStrength = FFX_CACAO_CLAMP(settings->shadowMultiplier * 4.3f, 0.0f, 10.0f);
   consts->EffectShadowPow = FFX_CACAO_CLAMP(settings->shadowPower, 0.0f, 10.0f);
   consts->EffectShadowClamp = FFX_CACAO_CLAMP(settings->shadowClamp, 0.0f, 1.0f);
   consts->EffectFadeOutMul = -1.0f / (settings->fadeOutTo - settings->fadeOutFrom);
   consts->EffectFadeOutAdd = settings->fadeOutFrom / (settings->fadeOutTo - settings->fadeOutFrom) + 1.0f;
   consts->EffectHorizonAngleThreshold = FFX_CACAO_CLAMP(settings->horizonAngleThreshold, 0.0f, 1.0f);

   // 1.2 seems to be around the best trade off - 1.0 means on-screen radius will stop/slow growing when the camera is at 1.0 distance, so, depending on FOV, basically filling up most of the screen
   // This setting is viewspace-dependent and not screen size dependent intentionally, so that when you change FOV the effect stays (relatively) similar.
   float effectSamplingRadiusNearLimit = (settings->radius * 1.2f);

   // if the depth precision is switched to 32bit float, this can be set to something closer to 1 (0.9999 is fine)
   consts->DepthPrecisionOffsetMod = 0.9992f;

   // consts->RadiusDistanceScalingFunctionPow     = 1.0f - CLAMP( m_settings.RadiusDistanceScalingFunction, 0.0f, 1.0f );


   // Special settings for lowest quality level - just nerf the effect a tiny bit
   if (settings->qualityLevel <= FFX_CACAO_QUALITY_LOW)
   {
      //consts.EffectShadowStrength     *= 0.9f;
      effectSamplingRadiusNearLimit *= 1.50f;

      if (settings->qualityLevel == FFX_CACAO_QUALITY_LOWEST)
         consts->EffectRadius *= 0.8f;
   }

   effectSamplingRadiusNearLimit /= tanHalfFOVY; // to keep the effect same regardless of FOV

   consts->EffectSamplingRadiusNearLimitRec = 1.0f / effectSamplingRadiusNearLimit;

   consts->AdaptiveSampleCountLimit = settings->adaptiveQualityLimit;

   consts->NegRecEffectRadius = -1.0f / consts->EffectRadius;

   consts->InvSharpness = FFX_CACAO_CLAMP(1.0f - settings->sharpness, 0.0f, 1.0f);

   consts->DetailAOStrength = settings->detailShadowStrength;

   // set buffer size constants.
   consts->SSAOBufferDimensions[0] = (float)bufferSizeInfo->ssaoBufferWidth;
   consts->SSAOBufferDimensions[1] = (float)bufferSizeInfo->ssaoBufferHeight;
   consts->SSAOBufferInverseDimensions[0] = 1.0f / ((float)bufferSizeInfo->ssaoBufferWidth);
   consts->SSAOBufferInverseDimensions[1] = 1.0f / ((float)bufferSizeInfo->ssaoBufferHeight);

   consts->DepthBufferDimensions[0] = (float)bufferSizeInfo->depthBufferWidth;
   consts->DepthBufferDimensions[1] = (float)bufferSizeInfo->depthBufferHeight;
   consts->DepthBufferInverseDimensions[0] = 1.0f / ((float)bufferSizeInfo->depthBufferWidth);
   consts->DepthBufferInverseDimensions[1] = 1.0f / ((float)bufferSizeInfo->depthBufferHeight);

   consts->DepthBufferOffset[0] = bufferSizeInfo->depthBufferXOffset;
   consts->DepthBufferOffset[1] = bufferSizeInfo->depthBufferYOffset;

   consts->InputOutputBufferDimensions[0] = (float)bufferSizeInfo->inputOutputBufferWidth;
   consts->InputOutputBufferDimensions[1] = (float)bufferSizeInfo->inputOutputBufferHeight;
   consts->InputOutputBufferInverseDimensions[0] = 1.0f / ((float)bufferSizeInfo->inputOutputBufferWidth);
   consts->InputOutputBufferInverseDimensions[1] = 1.0f / ((float)bufferSizeInfo->inputOutputBufferHeight);

   consts->ImportanceMapDimensions[0] = (float)bufferSizeInfo->importanceMapWidth;
   consts->ImportanceMapDimensions[1] = (float)bufferSizeInfo->importanceMapHeight;
   consts->ImportanceMapInverseDimensions[0] = 1.0f / ((float)bufferSizeInfo->importanceMapWidth);
   consts->ImportanceMapInverseDimensions[1] = 1.0f / ((float)bufferSizeInfo->importanceMapHeight);

   consts->DeinterleavedDepthBufferDimensions[0] = (float)bufferSizeInfo->deinterleavedDepthBufferWidth;
   consts->DeinterleavedDepthBufferDimensions[1] = (float)bufferSizeInfo->deinterleavedDepthBufferHeight;
   consts->DeinterleavedDepthBufferInverseDimensions[0] = 1.0f / ((float)bufferSizeInfo->deinterleavedDepthBufferWidth);
   consts->DeinterleavedDepthBufferInverseDimensions[1] = 1.0f / ((float)bufferSizeInfo->deinterleavedDepthBufferHeight
   );

   consts->DeinterleavedDepthBufferOffset[0] = (float)bufferSizeInfo->deinterleavedDepthBufferXOffset;
   consts->DeinterleavedDepthBufferOffset[1] = (float)bufferSizeInfo->deinterleavedDepthBufferYOffset;
   consts->DeinterleavedDepthBufferNormalisedOffset[0] = ((float)bufferSizeInfo->deinterleavedDepthBufferXOffset) / ((
      float)bufferSizeInfo->deinterleavedDepthBufferWidth);
   consts->DeinterleavedDepthBufferNormalisedOffset[1] = ((float)bufferSizeInfo->deinterleavedDepthBufferYOffset) / ((
      float)bufferSizeInfo->deinterleavedDepthBufferHeight);

   if (!settings->generateNormals)
   {
      consts->NormalsUnpackMul = 2.0f; // inputs->NormalsUnpackMul;
      consts->NormalsUnpackAdd = -1.0f; // inputs->NormalsUnpackAdd;
   }
   else
   {
      consts->NormalsUnpackMul = 2.0f;
      consts->NormalsUnpackAdd = -1.0f;
   }
}

void siRenderer::update()
{
   static siTimer timer;
   static float timeLeft = 0.f;
   float delta = timer.delta();
   timeLeft += delta * 0.000001f;

   camera.update();
   auto& cb = mainConstBuffer.get();
   cb.camPos = camera.position;
   cb.vpMatrix = camera.vpMatrix;
   mainConstBuffer.gpuCopy();

   auto& csCb = csConstBuffer.get();
   XMVECTOR det = XMMatrixDeterminant(XMLoadFloat4x4(&camera.vpMatrix));
   XMStoreFloat4x4(&csCb.vpMatrixInv, XMMatrixTranspose(XMMatrixInverse(&det, XMLoadFloat4x4(&camera.vpMatrix))));
   XMStoreFloat4x4(&csCb.vMatrix, camera.viewMatrix);
   csCb.width = window->getWidth();
   csCb.height = window->getHeight();
   FfxCacaoSettings settings = FFX_CACAO_DEFAULT_SETTINGS;
   FfxCacaoMatrix4x4 proj;
   FfxCacaoMatrix4x4 projInv;

   BufferSizeInfo bsi = {};
   bsi.inputOutputBufferWidth = window->getWidth();
   bsi.inputOutputBufferHeight = window->getHeight();
   bsi.depthBufferXOffset = 0;
   bsi.depthBufferYOffset = 0;
   bsi.depthBufferWidth = window->getWidth();
   bsi.depthBufferHeight = window->getHeight();

   bsi.ssaoBufferWidth = window->getWidth();
   bsi.ssaoBufferHeight = window->getHeight();
   bsi.deinterleavedDepthBufferXOffset = 0;
   bsi.deinterleavedDepthBufferYOffset = 0;
   bsi.deinterleavedDepthBufferWidth = window->getWidth();
   bsi.deinterleavedDepthBufferHeight = window->getHeight();
   bsi.importanceMapWidth = window->getWidth();
   bsi.importanceMapHeight = window->getHeight();

   memcpy(proj.elements, cb.vpMatrix.m, 16 * sizeof(float));
   memcpy(projInv.elements, csCb.vpMatrixInv.m, 16 * sizeof(float));

   updateConstants(&csCb.cacaoConst, &settings, &bsi, &proj, &projInv);

   csConstBuffer.gpuCopy();

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

   // drawing
   {
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
      commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
                                      normalsRenderTarget.getBuffer().Get(),
                                      D3D12_RESOURCE_STATE_RENDER_TARGET,
                                      D3D12_RESOURCE_STATE_COPY_SOURCE));
      commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
                                      renderTarget.getBuffer().Get(),
                                      D3D12_RESOURCE_STATE_RENDER_TARGET,
                                      D3D12_RESOURCE_STATE_COPY_DEST));
   }

   auto& depthPrepared = textures["#depthPrepared"];
   // compute shaders
   {
      computeShaders["depthPrepare"].dispatch(commandList.get(), window->getWidth(), window->getHeight());
   }

   // Copying compute shader results to render target
   {
      commandList->CopyResource(renderTarget.getBuffer().Get(), depthPrepared.getBuffer().Get());
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
