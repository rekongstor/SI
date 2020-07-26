#include "siRenderer.h"
#include "siWindow.h"
#include "siImgui.h"
#include "siSceneLoader.h"
#include "siTimer.h"
#include <random>

#include "cacao.h"

siRenderer::siRenderer(siWindow* window, uint32_t bufferCount):
   window(window),
   bufferCount(bufferCount),
   commandAllocator(bufferCount),
   descriptorMgr(10, 10, 300, 10),
   viewportScissor(window->getWidth(), window->getHeight()),
   camera({24.f, 12.f, 2.f, 1.f}, {0.f, 5.f, 17.f, 1.f}, 75.f,
          static_cast<float>(window->getWidth()) / static_cast<float>(window->getHeight()))
{
}

static void updateConstants(FfxCacaoConstants* consts, FfxCacaoSettings* settings, BufferSizeInfo* bufferSizeInfo,
                            const FfxCacaoMatrix4x4* proj)
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
      texture.initFromBuffer(swapChain.getBuffer(i), DXGI_FORMAT_UNKNOWN, window->getWidth(), window->getHeight());
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
         device.get(), window->getWidth(), window->getHeight(), 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM,
         D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON, sampleDesc);
      diffuse.createRtv(device.get(), &descriptorMgr);

      auto& normals = textures["#normalsRenderTarget"];
      normals.initTexture(
         device.get(), window->getWidth(), window->getHeight(), 1, 1, DXGI_FORMAT_R32G32B32A32_FLOAT,
         D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON, sampleDesc);
      normals.createRtv(device.get(), &descriptorMgr);
   }

   // creating const buffers
   {
      mainConstBuffer.initBuffer({}, device.get());
      ssaoConstBuffer.initBuffer({}, device.get());
      defRenderConstBuffer.initBuffer(
         {{}, {}, {0, 0, 0, 1}, {1, 1, 1, 1}, 4}, device.get());
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

   // Cacao initialization
   {
      uint32_t width = window->getWidth();
      uint32_t height = window->getHeight();
      uint32_t halfWidth = (width + 1) / 2;
      uint32_t halfHeight = (height + 1) / 2;
      uint32_t quarterWidth = (halfWidth + 1) / 2;
      uint32_t quarterHeight = (halfHeight + 1) / 2;
      uint32_t eighthWidth = (quarterWidth + 1) / 2;
      uint32_t eighthHeight = (quarterHeight + 1) / 2;

      uint32_t depthBufferWidth = width;
      uint32_t depthBufferHeight = height;
      uint32_t depthBufferHalfWidth = halfWidth;
      uint32_t depthBufferHalfHeight = halfHeight;
      uint32_t depthBufferQuarterWidth = quarterWidth;
      uint32_t depthBufferQuarterHeight = quarterHeight;

      uint32_t depthBufferXOffset = 0;
      uint32_t depthBufferYOffset = 0;
      uint32_t depthBufferHalfXOffset = 0;
      uint32_t depthBufferHalfYOffset = 0;
      uint32_t depthBufferQuarterXOffset = 0;
      uint32_t depthBufferQuarterYOffset = 0;

      BufferSizeInfo bsi = {};
      bsi.inputOutputBufferWidth = width;
      bsi.inputOutputBufferHeight = height;
      bsi.depthBufferXOffset = depthBufferXOffset;
      bsi.depthBufferYOffset = depthBufferYOffset;
      bsi.depthBufferWidth = depthBufferWidth;
      bsi.depthBufferHeight = depthBufferHeight;
      // not down-scaled version
      bsi.ssaoBufferWidth = halfWidth;
      bsi.ssaoBufferHeight = halfHeight;
      bsi.deinterleavedDepthBufferXOffset = depthBufferHalfXOffset;
      bsi.deinterleavedDepthBufferYOffset = depthBufferHalfYOffset;
      bsi.deinterleavedDepthBufferWidth = depthBufferHalfWidth;
      bsi.deinterleavedDepthBufferHeight = depthBufferHalfHeight;
      bsi.importanceMapWidth = quarterWidth;
      bsi.importanceMapHeight = quarterHeight;

      this->bsInfo = bsi;

      cacaoSettings = FFX_CACAO_DEFAULT_SETTINGS;
   }

   // Cacao prepare depths
   {
      auto& g_PrepareDepthsAndMips_OutMip0 = textures["#g_PrepareDepthsAndMips_OutMip0"];
      g_PrepareDepthsAndMips_OutMip0.initTexture(
         device.get(),
         bsInfo.deinterleavedDepthBufferWidth, bsInfo.deinterleavedDepthBufferHeight, 4, 1,
         DXGI_FORMAT_R16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, sampleDesc);
      auto& g_PrepareDepthsAndMips_OutMip1 = textures["#g_PrepareDepthsAndMips_OutMip1"];
      g_PrepareDepthsAndMips_OutMip1.initTexture(
         device.get(),
         bsInfo.deinterleavedDepthBufferWidth / 2, bsInfo.deinterleavedDepthBufferHeight / 2, 4, 1,
         DXGI_FORMAT_R16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, sampleDesc);
      auto& g_PrepareDepthsAndMips_OutMip2 = textures["#g_PrepareDepthsAndMips_OutMip2"];
      g_PrepareDepthsAndMips_OutMip2.initTexture(
         device.get(),
         bsInfo.deinterleavedDepthBufferWidth / 4, bsInfo.deinterleavedDepthBufferHeight / 4, 4, 1,
         DXGI_FORMAT_R16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, sampleDesc);
      auto& g_PrepareDepthsAndMips_OutMip3 = textures["#g_PrepareDepthsAndMips_OutMip3"];
      g_PrepareDepthsAndMips_OutMip3.initTexture(
         device.get(),
         bsInfo.deinterleavedDepthBufferWidth / 8, bsInfo.deinterleavedDepthBufferHeight / 8, 4, 1,
         DXGI_FORMAT_R16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, sampleDesc);

      auto& cacaoDownscaleDepth = computeShaders["cacaoPrepareDepths"];
      cacaoDownscaleDepth.onInit(device.get(), &descriptorMgr, L"cacaoPrepareDepths.hlsl",
                                 {
                                    textures["#depthStencil"]
                                 },
                                 {
                                    g_PrepareDepthsAndMips_OutMip0, g_PrepareDepthsAndMips_OutMip1,
                                    g_PrepareDepthsAndMips_OutMip2, g_PrepareDepthsAndMips_OutMip3
                                 },
                                 ssaoConstBuffer.getGpuVirtualAddress());
   }

   // Cacao prepare normals
   {
      auto& deinterlacedNormals = textures["#deinterlacedNormals"];
      deinterlacedNormals.initTexture(
         device.get(), bsInfo.ssaoBufferWidth, bsInfo.ssaoBufferHeight, 4, 1, DXGI_FORMAT_R8G8B8A8_SNORM,
         D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, sampleDesc);
      auto& cacaoDownscaleDepth = computeShaders["cacaoPrepareNormals"];
      cacaoDownscaleDepth.onInit(device.get(), &descriptorMgr, L"cacaoPrepareNormals.hlsl",
                                 {
                                    textures["#normalsRenderTarget"]
                                 },
                                 {
                                    deinterlacedNormals
                                 },
                                 ssaoConstBuffer.getGpuVirtualAddress());
   }

   // Cacao first SSAO pass
   {
      auto& g_LoadCounter = textures["#g_LoadCounter"];
      g_LoadCounter.initTexture(
         device.get(), 1, 0, 1, 1, DXGI_FORMAT_R32_UINT,
         D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, sampleDesc);

      auto& g_ImportanceMap = textures["#g_ImportanceMap"];
      g_ImportanceMap.initTexture(
         device.get(), bsInfo.importanceMapWidth, bsInfo.importanceMapHeight, 1, 1, DXGI_FORMAT_R8_UNORM,
         D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, sampleDesc);

      auto& g_FinalSSAO = textures["#g_FinalSSAO"];
      g_FinalSSAO.initTexture(
         device.get(), bsInfo.ssaoBufferWidth, bsInfo.ssaoBufferHeight, 4, 1, DXGI_FORMAT_R8G8_UNORM,
         D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, sampleDesc);

      auto& cacaoSSAOq3Base = computeShaders["cacaoSSAOq3Base"];
      cacaoSSAOq3Base.onInit(device.get(), &descriptorMgr, L"cacaoSSAOq3Base.hlsl",
                             {
                                textures["#depthStencil"],
                                textures["#normalsRenderTarget"],
                                textures["#g_LoadCounter"],
                                textures["#g_ImportanceMap"],
                                textures["#g_FinalSSAO"],
                                textures["#deinterlacedNormals"],
                             },
                             {
                                textures["#g_FinalSSAO"]
                             },
                             ssaoConstBuffer.getGpuVirtualAddress());
   }

   // creating compute shaders
   {
      auto& texture = textures["#deferredRenderTarget"];
      texture.initTexture(
         device.get(), window->getWidth(), window->getHeight(), 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM,
         D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, sampleDesc);

      auto& deferredRender = computeShaders["deferredRender"];
      deferredRender.onInit(device.get(), &descriptorMgr, L"pbrRender.hlsl",
                            {
                               textures["#diffuseRenderTarget"], textures["#depthStencil"],
                               textures["#normalsRenderTarget"], textures["#g_FinalSSAO"]
                            },
                            {texture},
                            defRenderConstBuffer.getGpuVirtualAddress());
   }

   siSceneLoader::loadScene("sponza.obj", meshes, textures, device.get(), commandList, &descriptorMgr);

   // creating instances
   for (int i = 0; i < meshes.size(); ++i)
   {
      auto& inst = instances[i];
      const uint32_t x = 1;
      const uint32_t y = 1;
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
            DirectX::XMStoreFloat4x4(&instanceData.world, world);
            DirectX::XMStoreFloat4x4(&instanceData.worldIt, InverseTranspose(world));

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
   DirectX::XMStoreFloat4x4(&mainCb.viewMatrix, camera.viewMatrix);
   DirectX::XMStoreFloat4x4(&mainCb.projMatrix, camera.projMatrix);
   mainConstBuffer.gpuCopy();

   auto& ssaoCb = ssaoConstBuffer.get();

   FfxCacaoMatrix4x4 proj;
   memcpy(proj.elements, mainCb.projMatrix.m, sizeof(float) * 16);
   updateConstants(&ssaoCb.consts, &this->cacaoSettings, &this->bsInfo, &proj);
   ssaoConstBuffer.gpuCopy();

   auto& defRenCb = defRenderConstBuffer.get();
   float4 lightDirection = {0.f, -1.f, 0.f, 0.f};
   XMStoreFloat4(&defRenCb.lightDirection,
                 XMVector4Transform(XMLoadFloat4(&lightDirection), (camera.viewMatrix)));
   defRenCb.targetOutput = targetOutput;
   defRenCb.width = window->getWidth();
   defRenCb.height = window->getHeight();
   auto det = XMMatrixDeterminant(camera.projMatrix);
   auto projInv = XMMatrixInverse(&det, camera.projMatrix);
   DirectX::XMStoreFloat4x4(&defRenCb.projMatrixInv, projInv);
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
   const float clearColorWhite[] = {1.0f, 1.0f, 1.0f, 1.0f};
   auto& swapChainTarget = swapChainTargets[currentFrame];
   auto& depthStencil = textures["#depthStencil"];

   auto& normalsRenderTarget = textures["#normalsRenderTarget"];
   auto& diffuseRenderTarget = textures["#diffuseRenderTarget"];

   auto& deferredRenderTarget = textures["#deferredRenderTarget"];

   // drawing
   {
      diffuseRenderTarget.resourceBarrier(commandList.get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
      normalsRenderTarget.resourceBarrier(commandList.get(), D3D12_RESOURCE_STATE_RENDER_TARGET);

      D3D12_CPU_DESCRIPTOR_HANDLE rts[] = {
         diffuseRenderTarget.getRtvHandle().first,
         normalsRenderTarget.getRtvHandle().first
      };
      commandList->OMSetRenderTargets(_countof(rts),
                                      rts,
                                      FALSE,
                                      &depthStencil.getDsvHandle().first);


      commandList->ClearRenderTargetView(diffuseRenderTarget.getRtvHandle().first, clearColor, 0, nullptr);
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
      normalsRenderTarget.resourceBarrier(commandList.get(), D3D12_RESOURCE_STATE_COMMON);
   }

   // compute shaders
   {
      computeShaders["cacaoPrepareDepths"].dispatch(commandList.get());
      computeShaders["cacaoPrepareNormals"].dispatch(commandList.get());
      computeShaders["cacaoSSAOq3Base"].dispatch(commandList.get());
      computeShaders["deferredRender"].dispatch(commandList.get());
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
