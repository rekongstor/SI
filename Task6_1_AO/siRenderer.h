#pragma once
#include "siCamera.h"
#include "siDevice.h"
#include "siCommandQueue.h"
#include "siCommandAllocator.h"
#include "siCommandList.h"
#include "siConstBuffer.h"
#include "siSwapChain.h"
#include "siTexture2D.h"
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

   siTexture2D swapChainTargets[maxFrameBufferCount];
   std::map<std::string, siTexture2D> textures;
   std::map<std::string, siRootSignature> rootSignatures;
   std::map<std::string, siPipelineState> pipelineStates;
   std::map<int32_t, siMesh> meshes;
   std::map<int32_t, siInstance> instances;

   siCamera camera;

   siConstBuffer<mainConstBuff> mainConstBuffer[maxFrameBufferCount];

   void update();
   void updatePipeline();
   void executePipeline();
public:
   explicit siRenderer(siWindow* window, uint32_t bufferCount);

   [[nodiscard]] bool isActive() const;
   void onInit(siImgui* imgui = nullptr);
   void onUpdate();
};
