#pragma once
#include "rnd_TextureMgr.h"
#include "rnd_SwapChainMgr.h"
#include "rnd_DescriptorHeapMgr.h"
#include "rnd_CommandMgr.h"
#include "rnd_RayTracingPipeline.h"
class core_Window;

class rnd_Dx12
{
public:
   // Global functions
   void EnableGpuValidation();
   void OnInit(core_Window* window);
   void OnUpdate();

   // Graphics
   void ExecuteCommandList();
   void WaitForGpu();
   void PopulateGraphicsCommandList();
   void MoveToNextFrame();

   // Core window
   D3D12_VIEWPORT viewport;
   D3D12_RECT scissorRect;
   core_Window* window;

   // Device
   ComPtr<IDXGIFactory6> factory;
   ComPtr<IDXGIAdapter1> adapter;
   ComPtr<ID3D12Device> device;

   // Command list

   // Fences
   ComPtr<ID3D12Fence1> fence;
   uint64_t fenceValues[2];
   Event fenceEvent;

   // Managers
   rnd_SwapChainMgr swapChainMgr;
   rnd_DescriptorHeapMgr descriptorHeapMgr;
   rnd_TextureMgr textureMgr;
   rnd_CommandMgr commandMgr;
   rnd_RayTracingPipeline rayTracingPipeline;

   int currentFrame = 0;
   int syncInterval = 0; // UINT_MAX for V-Sync
   bool windowed = true;
   bool rtxSupported = false;
   bool tearingSupported = false;

};

