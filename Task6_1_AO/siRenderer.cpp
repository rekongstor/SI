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
   camera({5.16772985, 1.89779234, -1.41415465f, 1.f}, {0.703276634f, 1.02280307f, 0.218072295f, 1.f}, 45.f,
          static_cast<float>(window->getWidth()) / static_cast<float>(window->getHeight()), 0.1f, 250000.0f)
{
   targetOutput = 4;
   targetArray = 0;
   targetMip = 0;
   cacaoSsao = 0;
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


   consts->NormalsUnpackMul = 2.0f;
   consts->NormalsUnpackAdd = -1.0f;
}

static void updatePerPassConstants(FfxCacaoConstants* consts, FfxCacaoSettings* settings,
                                   BufferSizeInfo* bufferSizeInfo, int pass)
{
   consts->PerPassFullResUVOffset[0] = ((float)(pass % 2)) / (float)bufferSizeInfo->ssaoBufferWidth;
   consts->PerPassFullResUVOffset[1] = ((float)(pass / 2)) / (float)bufferSizeInfo->ssaoBufferHeight;

   consts->PassIndex = pass;

   float additionalAngleOffset = settings->temporalSupersamplingAngleOffset;
   // if using temporal supersampling approach (like "Progressive Rendering Using Multi-frame Sampling" from GPU Pro 7, etc.)
   float additionalRadiusScale = settings->temporalSupersamplingRadiusOffset;
   // if using temporal supersampling approach (like "Progressive Rendering Using Multi-frame Sampling" from GPU Pro 7, etc.)
   const int subPassCount = 5;
   for (int subPass = 0; subPass < subPassCount; subPass++)
   {
      int a = pass;
      int b = subPass;

      int spmap[5]{0, 1, 4, 3, 2};
      b = spmap[subPass];

      float ca, sa;
      float angle0 = ((float)a + (float)b / (float)subPassCount) * (3.1415926535897932384626433832795f) * 0.5f;
      // angle0 += additionalAngleOffset;

      ca = FFX_CACAO_COS(angle0);
      sa = FFX_CACAO_SIN(angle0);

      float scale = 1.0f + (a - 1.5f + (b - (subPassCount - 1.0f) * 0.5f) / (float)subPassCount) * 0.07f;
      // scale *= additionalRadiusScale;

      consts->PatternRotScaleMatrices[subPass][0] = scale * ca;
      consts->PatternRotScaleMatrices[subPass][1] = scale * -sa;
      consts->PatternRotScaleMatrices[subPass][2] = -scale * sa;
      consts->PatternRotScaleMatrices[subPass][3] = -scale * ca;
   }
}


void siRenderer::onInit(siImgui* imgui)
{
   std::cout << "Initializing renderer..." << std::endl;
   HRESULT hr = S_OK;

   // Debug
   {
      hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debugController0));
      assert(hr == S_OK);
      debugController0->QueryInterface(IID_PPV_ARGS(&debugController1));
      debugController1->SetEnableGPUBasedValidation(true);
   }

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
                            descriptorMgr.getCbvSrvUavHandle(), this);
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
      depthStencilTarget.initDepthStencil(device.get(), window->getWidth(), window->getHeight(), sampleDesc);
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
         device.get(), window->getWidth(), window->getHeight(), 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM,
         D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON, sampleDesc);
      normals.createRtv(device.get(), &descriptorMgr);
   }

   // creating const buffers
   {
      mainConstBuffer.initBuffer({}, device.get());
      ssaoConstBuffer[0].initBuffer({}, device.get());
      ssaoConstBuffer[1].initBuffer({}, device.get());
      ssaoConstBuffer[2].initBuffer({}, device.get());
      ssaoConstBuffer[3].initBuffer({}, device.get());
      //defaultSsaoConstBuffer.initBuffer({{}, {}, {}, {}, 0.5, 0.025}, device.get());
      defRenderConstBuffer.initBuffer(
         {{}, {}, {0, 0, 0, 1}, {1, 1, 1, 1}}, device.get());


      float4 sas;
      sas.x = tanf((45.f / 180.f * 3.14f) * 0.5f);
      sas.y = tanf((45.f / 180.f * 3.14f) * 0.5f);
      sas.z = 1.f;
      sas.w = sas.z * 1.f;
      siSsaoBuffer.initBuffer({
                                 {}, {
                                    1.f,
                                    16.f,
                                    1.f / 16.f,
                                    1.f
                                 },
                                 {}, {}, {},
                                 {sas.x * 2.f, sas.y * 2.f, 0.5f / sas.x, 0.5f / sas.y},
                                 {sas.z * 2.f, sas.w * 2.f, 0.5f / sas.z, 0.5f / sas.w},
                                 {2, 0, 0, 0}
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

   // Cacao resources and CS
   {
      // Cacao prepare depths
      auto& g_PrepareDepthsAndMips = textures["#g_PrepareDepthsAndMips"];
      g_PrepareDepthsAndMips.initTexture(
         device.get(),
         bsInfo.deinterleavedDepthBufferWidth, bsInfo.deinterleavedDepthBufferHeight, 4, 4,
         DXGI_FORMAT_R16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, sampleDesc);

      auto& cacaoDownscaleDepth = computeShaders["cacaoPrepareDepths"];
      cacaoDownscaleDepth.onInit(device.get(), &descriptorMgr, L"cacaoPrepareDepths.hlsl",
                                 {
                                    textures["#depthStencil"]
                                 },
                                 {
                                    g_PrepareDepthsAndMips
                                 },
                                 ssaoConstBuffer[0].getGpuVirtualAddress());


      // Cacao prepare normals

      auto& deinterlacedNormals = textures["#deinterlacedNormals"];
      deinterlacedNormals.initTexture(
         device.get(), bsInfo.ssaoBufferWidth, bsInfo.ssaoBufferHeight, 4, 1, DXGI_FORMAT_R8G8B8A8_SNORM,
         D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, sampleDesc);
      auto& cacaoPrepareNormals = computeShaders["cacaoPrepareNormals"];
      cacaoPrepareNormals.onInit(device.get(), &descriptorMgr, L"cacaoPrepareNormals.hlsl",
                                 {
                                    textures["#normalsRenderTarget"]
                                 },
                                 {
                                    deinterlacedNormals
                                 },
                                 ssaoConstBuffer[0].getGpuVirtualAddress());


      // Cacao first SSAO pass

      auto& g_LoadCounter = textures["#g_LoadCounter"];
      g_LoadCounter.initTexture(
         device.get(), 1, 0, 1, 1, DXGI_FORMAT_R32_UINT,
         D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, sampleDesc);
      g_LoadCounter.createUav(device.get(), &descriptorMgr);

      auto& g_ImportanceMap = textures["#g_ImportanceMap"];
      g_ImportanceMap.initTexture(
         device.get(), bsInfo.importanceMapWidth, bsInfo.importanceMapHeight, 1, 1, DXGI_FORMAT_R8_UNORM,
         D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, sampleDesc);

      auto& g_ImportanceMapPong = textures["#g_ImportanceMapPong"];
      g_ImportanceMapPong.initTexture(
         device.get(), bsInfo.importanceMapWidth, bsInfo.importanceMapHeight, 1, 1, DXGI_FORMAT_R8_UNORM,
         D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, sampleDesc);

      auto& g_FinalSSAO = textures["#g_FinalSSAO"];
      g_FinalSSAO.initTexture(
         device.get(), bsInfo.ssaoBufferWidth, bsInfo.ssaoBufferHeight, 4, 1, DXGI_FORMAT_R8G8_UNORM,
         D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, sampleDesc);

      auto& g_FinalSSAOBlurPong = textures["#g_FinalSSAOBlurPong"];
      g_FinalSSAOBlurPong.initTexture(
         device.get(), bsInfo.ssaoBufferWidth, bsInfo.ssaoBufferHeight, 4, 1, DXGI_FORMAT_R8G8_UNORM,
         D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, sampleDesc);

      auto& g_SSAOOutput = textures["#g_SSAOOutput"];
      g_SSAOOutput.initTexture(
         device.get(), window->getWidth(), window->getHeight(), 1, 1, DXGI_FORMAT_R8_UNORM,
         D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, sampleDesc);

      auto& cacaoSSAOq3Base = computeShaders["cacaoSSAOq3Base"];
      cacaoSSAOq3Base.onInit(device.get(), &descriptorMgr, L"cacaoSSAOq3Base.hlsl",
                             {
                                g_PrepareDepthsAndMips,
                                deinterlacedNormals,
                                g_LoadCounter,
                                g_ImportanceMap,
                                g_FinalSSAO,
                                deinterlacedNormals,
                             },
                             {
                                g_FinalSSAO
                             },
                             ssaoConstBuffer[0].getGpuVirtualAddress());

      auto& cacaoGenerateImportanceMap = computeShaders["cacaoGenerateImportanceMap"];
      cacaoGenerateImportanceMap.onInit(device.get(), &descriptorMgr, L"cacaoGenerateImportanceMap.hlsl",
                                        {
                                           g_FinalSSAO,
                                        },
                                        {
                                           g_ImportanceMap
                                        },
                                        ssaoConstBuffer[0].getGpuVirtualAddress());

      auto& cacaoPostprocessImportanceA = computeShaders["cacaoPostprocessImportanceA"];
      cacaoPostprocessImportanceA.onInit(device.get(), &descriptorMgr, L"cacaoPostprocessImportanceA.hlsl",
                                         {
                                            g_ImportanceMap,
                                         },
                                         {
                                            g_ImportanceMapPong
                                         },
                                         ssaoConstBuffer[0].getGpuVirtualAddress());

      auto& cacaoPostprocessImportanceB = computeShaders["cacaoPostprocessImportanceB"];
      cacaoPostprocessImportanceB.onInit(device.get(), &descriptorMgr, L"cacaoPostprocessImportanceB.hlsl",
                                         {
                                            g_ImportanceMapPong,
                                         },
                                         {
                                            g_ImportanceMap,
                                            g_LoadCounter,
                                         },
                                         ssaoConstBuffer[0].getGpuVirtualAddress());


      auto& cacaoSSAOq3 = computeShaders["cacaoSSAOq3"];
      cacaoSSAOq3.onInit(device.get(), &descriptorMgr, L"cacaoSSAOq3.hlsl",
                         {
                            g_PrepareDepthsAndMips,
                            deinterlacedNormals,
                            g_LoadCounter,
                            g_ImportanceMap,
                            g_FinalSSAO,
                            deinterlacedNormals,
                         },
                         {
                            g_FinalSSAO
                         },
                         ssaoConstBuffer[0].getGpuVirtualAddress());


      const char* name[] = {
         "cacaoBlur1",
         "cacaoBlur2",
         "cacaoBlur3",
         "cacaoBlur4",
         "cacaoBlur5",
         "cacaoBlur6",
         "cacaoBlur7",
         "cacaoBlur8",
      };
      const wchar_t* filename[] = {
         L"cacaoBlur1.hlsl",
         L"cacaoBlur2.hlsl",
         L"cacaoBlur3.hlsl",
         L"cacaoBlur4.hlsl",
         L"cacaoBlur5.hlsl",
         L"cacaoBlur6.hlsl",
         L"cacaoBlur7.hlsl",
         L"cacaoBlur8.hlsl",
      };
      for (auto i = 0; i < _countof(filename); ++i)
      {
         auto& cacaoBlur = computeShaders[name[i]];
         cacaoBlur.onInit(device.get(), &descriptorMgr, filename[i],
                          {
                             g_FinalSSAO,
                          },
                          {
                             g_FinalSSAOBlurPong
                          },
                          ssaoConstBuffer[0].getGpuVirtualAddress());
      }

      auto& cacaoApply = computeShaders["cacaoApply"];
      cacaoApply.onInit(device.get(), &descriptorMgr, L"cacaoApply.hlsl",
                        {
                           g_FinalSSAO,
                        },
                        {
                           g_SSAOOutput,
                        },
                        ssaoConstBuffer[0].getGpuVirtualAddress());


      auto& cacaoApplyBlurred = computeShaders["cacaoApplyBlurred"];
      cacaoApplyBlurred.onInit(device.get(), &descriptorMgr, L"cacaoApply.hlsl",
                               {
                                  g_FinalSSAOBlurPong,
                               },
                               {
                                  g_SSAOOutput,
                               },
                               ssaoConstBuffer[0].getGpuVirtualAddress());


      auto& ssaoOutputTex = textures["#ssaoOutputTex"];
      ssaoOutputTex.initTexture(
         device.get(), window->getWidth(), window->getHeight(), 1, 1, DXGI_FORMAT_R8_UNORM,
         D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, sampleDesc);
      auto& SSAOOutputBlur = textures["#SSAOOutputBlur"];
      SSAOOutputBlur.initTexture(
         device.get(), window->getWidth(), window->getHeight(), 1, 1, DXGI_FORMAT_R8_UNORM,
         D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, sampleDesc);


      auto& ssaoSiOutput = textures["#ssaoSiOutput"];
      ssaoSiOutput.initTexture(
         device.get(), window->getWidth(), window->getHeight(), 1, 1, DXGI_FORMAT_R16G16B16A16_FLOAT,
         D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, sampleDesc);
      auto& ssaoSi = computeShaders["ssaoSi"];
      ssaoSi.onInit(device.get(), &descriptorMgr, L"ssaoSi.hlsl",
                    {
                       textures["#depthStencil"], textures["#normalsRenderTarget"], textures["#diffuseRenderTarget"]
                    },
                    {
                       ssaoSiOutput
                    },
                    siSsaoBuffer.getGpuVirtualAddress());
      auto& ssaoSiApply = computeShaders["ssaoSiApply"];
      ssaoSiApply.onInit(device.get(), &descriptorMgr, L"ssaoSiApply.hlsl",
                         {
                            textures["#depthStencil"], ssaoSiOutput
                         },
                         {
                            g_SSAOOutput
                         },
                         siSsaoBuffer.getGpuVirtualAddress());

      auto& texture = textures["#deferredRenderTarget"];
      texture.initTexture(
         device.get(), window->getWidth(), window->getHeight(), 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM,
         D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, sampleDesc);

      auto& deferredRender = computeShaders["deferredRender"];
      deferredRender.onInit(device.get(), &descriptorMgr, L"pbrRender.hlsl",
                            {
                               textures["#diffuseRenderTarget"], textures["#depthStencil"],
                               textures["#normalsRenderTarget"], g_SSAOOutput,
                               ssaoSiOutput
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
      float scale = 1.0f;
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

   gpuTimerInit(&timer, device.get(), bufferCount);

   fenceMgr.waitForPreviousFrame(currentFrame);
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
   mainCb.cacaoSsao = cacaoSsao;
   mainConstBuffer.gpuCopy();

   auto& defRenCb = defRenderConstBuffer.get();
   float4 lightDirection = {0.f, -1.f, 0.f, 0.f};
   XMStoreFloat4(&defRenCb.lightDirection,
                 XMVector4Transform(XMLoadFloat4(&lightDirection), (camera.viewMatrix)));
   defRenCb.targetOutput = targetOutput;
   defRenCb.width = window->getWidth();
   defRenCb.height = window->getHeight();
   defRenCb.targetMip = targetMip;
   defRenCb.targetArray = targetArray;
   auto det = XMMatrixDeterminant(camera.projMatrix);
   auto projInv = (XMMatrixInverse(&det, camera.projMatrix));
   DirectX::XMStoreFloat4x4(&defRenCb.projMatrixInv, projInv);
   defRenderConstBuffer.gpuCopy();

   //auto& dSsao = defaultSsaoConstBuffer.get();
   //dSsao.width = window->getWidth();
   //dSsao.height = window->getHeight();
   //dSsao.widthInv = 1.f / window->getWidth();
   //dSsao.heightInv = 1.f / window->getHeight();
   //dSsao.projMatrixInv = defRenCb.projMatrixInv;
   //dSsao.projMatrix = mainCb.projMatrix;
   //defaultSsaoConstBuffer.gpuCopy();


   auto& ssaoCb = ssaoConstBuffer[0].get();
   FfxCacaoMatrix4x4 proj;
   memcpy(proj.elements, mainCb.projMatrix.m, sizeof(float) * 16);
   updateConstants(&ssaoCb.consts, &cacaoSettings, &bsInfo, &proj, &FFX_CACAO_IDENTITY_MATRIX);
   updatePerPassConstants(&ssaoCb.consts, &cacaoSettings, &bsInfo, 0);
   ssaoConstBuffer[0].gpuCopy();
   for (int i = 1; i < 4; ++i)
   {
      auto& ssaoCbPerPass = ssaoConstBuffer[i].get();
      ssaoCbPerPass.consts = ssaoCb.consts;
      updatePerPassConstants(&ssaoCbPerPass.consts, &cacaoSettings, &bsInfo, i);
      ssaoConstBuffer[i].gpuCopy();
   }

   auto& siSsaoCb = siSsaoBuffer.get();
   siSsaoCb.PS_REG_SSAO_SCREEN = {
      1.f / window->getWidth(), 1.f / window->getHeight(), 0.5f / window->getHeight(), 0.5f / window->getHeight()
   };
   siSsaoCb.PS_REG_SSAO_MV_1 =
      {mainCb.viewMatrix.m[0][0], mainCb.viewMatrix.m[1][0], mainCb.viewMatrix.m[2][0], 1.f};
   siSsaoCb.PS_REG_SSAO_MV_2 =
      {-mainCb.viewMatrix.m[0][1], -mainCb.viewMatrix.m[1][1], -mainCb.viewMatrix.m[2][1], 1.f};
   siSsaoCb.PS_REG_SSAO_MV_3 =
      {mainCb.viewMatrix.m[0][2], mainCb.viewMatrix.m[1][2], mainCb.viewMatrix.m[2][2], 1.f};
   //siSsaoCb.PS_REG_SSAO_MV_1 = {1, 0, 0, 1};
   //siSsaoCb.PS_REG_SSAO_MV_2 = {0, 1, 0, 1};
   //siSsaoCb.PS_REG_SSAO_MV_3 = {0, 0, 1, 1};
   siSsaoBuffer.gpuCopy();

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
   gpuTimerStartFrame(&timer);
   {
      GET_TIMESTAMP(&timer, commandList.get(), "Begin frame");
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
      GET_TIMESTAMP(&timer, commandList.get(), "Drawing");
   }

   // compute shaders
   {
      // Cacao
      if (cacaoSsao)
      {
         UINT clearValue[] = {0, 0, 0, 0};
         auto& loadCounter = textures["#g_LoadCounter"];
         commandList->ClearUnorderedAccessViewUint(loadCounter.getUavHandle().second, loadCounter.getUavHandle().first,
                                                   loadCounter.getBuffer().Get(), clearValue, 0, NULL);


         computeShaders["cacaoPrepareDepths"].dispatch(commandList.get());
         computeShaders["cacaoPrepareNormals"].dispatch(commandList.get());
         for (int i = 0; i < 4; ++i)
            computeShaders["cacaoSSAOq3Base"].dispatch(commandList.get(), ssaoConstBuffer[i].getGpuVirtualAddress());

         computeShaders["cacaoGenerateImportanceMap"].dispatch(commandList.get());
         computeShaders["cacaoPostprocessImportanceA"].dispatch(commandList.get());
         computeShaders["cacaoPostprocessImportanceB"].dispatch(commandList.get());

         for (int i = 0; i < 4; ++i)
            computeShaders["cacaoSSAOq3"].dispatch(commandList.get(), ssaoConstBuffer[i].getGpuVirtualAddress());

         int blurPassCount = this->cacaoSettings.blurPassCount;
         blurPassCount = FFX_CACAO_CLAMP(blurPassCount, 0, MAX_BLUR_PASSES);

         if (blurPassCount)
         {
            uint32_t w = 4 * BLUR_WIDTH - 2 * blurPassCount;
            uint32_t h = 3 * BLUR_HEIGHT - 2 * blurPassCount;
            uint32_t dispatchWidth = dispatchSize(w, bsInfo.ssaoBufferWidth);
            uint32_t dispatchHeight = dispatchSize(h, bsInfo.ssaoBufferHeight);
            char name[] = "cacaoBlur0";
            name[9] = ('0' + blurPassCount);
            for (int i = 0; i < 4; ++i)
            {
               computeShaders[name].dispatch(commandList.get(), ssaoConstBuffer[i].getGpuVirtualAddress(),
                                             dispatchWidth, dispatchHeight);
            }
            computeShaders["cacaoApplyBlurred"].dispatch(commandList.get());
         }
         else
         {
            computeShaders["cacaoApply"].dispatch(commandList.get());
         }
      }
      else
      {
         computeShaders["ssaoSi"].dispatch(commandList.get());
         computeShaders["ssaoSiApply"].dispatch(commandList.get());
      }
      GET_TIMESTAMP(&timer, commandList.get(), "SSAO");
      computeShaders["deferredRender"].dispatch(commandList.get());
      GET_TIMESTAMP(&timer, commandList.get(), "Rendering");
   }

   // Copying compute shader results to render target
   {
      swapChainTarget.resourceBarrier(commandList.get(), D3D12_RESOURCE_STATE_COPY_DEST);
      deferredRenderTarget.resourceBarrier(commandList.get(), D3D12_RESOURCE_STATE_COPY_SOURCE);
      commandList->CopyResource(swapChainTarget.getBuffer().Get(), deferredRenderTarget.getBuffer().Get());
      swapChainTarget.resourceBarrier(commandList.get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
      deferredRenderTarget.resourceBarrier(commandList.get(), D3D12_RESOURCE_STATE_COMMON);
      GET_TIMESTAMP(&timer, commandList.get(), "Updating swap chain");
   }

   // imgui draw
   {
      commandList->OMSetRenderTargets(1,
                                      &swapChainTarget.getRtvHandle().first,
                                      FALSE,
                                      &depthStencil.getDsvHandle().first);
      if (imgui)
         imgui->onRender(commandList.get());
      GET_TIMESTAMP(&timer, commandList.get(), "Imgui");
   }

   swapChainTarget
      .
      resourceBarrier(commandList
                      .
                      get(), D3D12_RESOURCE_STATE_PRESENT
      );

   gpuTimerEndFrame(&timer, commandList.get());
}

void siRenderer::getTimings(FfxCacaoDetailedTiming* timings, uint64_t* gpuTicksPerSecond)
{
   gpuTimerCollectTimings(&timer, timings);
   commandQueue.get().Get()->GetTimestampFrequency(gpuTicksPerSecond);
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
