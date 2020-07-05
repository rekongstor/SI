#pragma once
#include "siDevice.h"
#include "siCommandQueue.h"
#include "siCommandAllocator.h"
#include "siCommandList.h"
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
   uint32_t currentFrame;
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
   siTexture2D depthStencilTarget;
   std::map<std::string, siTexture2D> textures;
   std::map<int32_t, siRootSignature> rootSignatures;
   std::map<int32_t, siPipelineState> pipelineStates;
   std::map<int32_t, siMesh> meshes;
   std::vector<siInstance> instances;

   void UpdatePipeline();
public:
   explicit siRenderer(siWindow* window, uint32_t bufferCount);

   [[nodiscard]] bool isActive() const;
   void onInit(siImgui* imgui = nullptr);
   void onUpdate();
   void onDestroy();
};
