#pragma once
#include "cacao.h"
#include "siCamera.h"
#include "siDevice.h"
#include "siCommandQueue.h"
#include "siCommandAllocator.h"
#include "siCommandList.h"
#include "siComputeShader.h"
#include "siConstBuffer.h"
#include "siSwapChain.h"
#include "siTexture.h"
#include "siDescriptorMgr.h"
#include "siFenceMgr.h"
#include "siInstance.h"
#include "siMesh.h"
#include "siPipelineState.h"
#include "siRootSignature.h"
#include "siViewportScissor.h"

class siWindow;
class siImgui;

class siRenderer
{
   friend class siImgui;
   siWindow* window = nullptr;
   siImgui* imgui = nullptr;
   uint32_t bufferCount;
   uint32_t currentFrame = 0;
   DXGI_SAMPLE_DESC sampleDesc = {1, 0};
   bool active = false;

   static const uint32_t frameBuffers = 3;
   static const D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;
   static const bool softwareAdapter = false;

   siDevice device;
   siCommandQueue commandQueue;
   siCommandAllocator commandAllocator;
   siCommandList commandList;
   siSwapChain swapChain;
   siDescriptorMgr descriptorMgr;
   siFenceMgr fenceMgr;

   siViewportScissor viewportScissor;

   siTexture swapChainTargets[maxFrameBufferCount];
   std::map<std::string, siTexture> textures;
   std::map<std::string, siRootSignature> rootSignatures;
   std::map<std::string, siPipelineState> pipelineStates;
   std::map<int32_t, siMesh> meshes;
   std::map<int32_t, siInstance> instances;
   std::map<std::string, siComputeShader> computeShaders;

   siCamera camera;
   int targetOutput;
   int targetArray;
   int targetMip;
   bool cacaoSsao;
   bool wasdMovement;

   siConstBuffer<mainConstBuff> mainConstBuffer;
   siConstBuffer<ssaoConstBuff> ssaoConstBuffer[4];
   siConstBuffer<defRenderConstBuff> defRenderConstBuffer;
   siConstBuffer<siSsaoBuff> siSsaoBuffer;

   FfxCacaoSettings cacaoSettings;
   BufferSizeInfo bsInfo;

   GpuTimer timer;
   void getTimings(FfxCacaoDetailedTiming* timings, uint64_t* gpuTicksPerSecond);

   ComPtr<ID3D12Debug> debugController0;
   ComPtr<ID3D12Debug1> debugController1;

   void update();
   void updatePipeline();
   void executePipeline();
public:
   explicit siRenderer(siWindow* window, uint32_t bufferCount);

   [[nodiscard]] bool isActive() const;
   void onInit(siImgui* imgui = nullptr);
   void onUpdate();
};
