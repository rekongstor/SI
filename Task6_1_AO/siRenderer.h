#pragma once
#include "siDevice.h"
#include "siCommandQueue.h"
#include "siCommandAllocator.h"
#include "siCommandList.h"
#include "siSwapChain.h"
#include "siTexture2D.h"
#include "siDescriptorMgr.h"
#include "siFenceMgr.h"

class siWindow;

class siRenderer
{
   siWindow* window = nullptr;
   uint32_t bufferCount;
   uint32_t currentFrame;
   DXGI_SAMPLE_DESC sampleDesc = { 1, 0 };
   bool active = false;

   static const uint32_t frameBuffers = 3;
   static const D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_12_1;
   static const bool softwareAdapter = false;

   siDevice device;
   siCommandQueue commandQueue;
   siCommandAllocator commandAllocator;
   siCommandList commandList;
   siSwapChain swapChain;
   siDescriptorMgr descriptorMgr;
   siFenceMgr fenceMgr;
   std::map<int32_t, siTexture2D> textures;

   void UpdatePipeline();
public:
   explicit siRenderer(siWindow* window, uint32_t bufferCount);

   [[nodiscard]] bool isActive() const;
   void onInit();
   void onUpdate();
   void onDestroy();
};
