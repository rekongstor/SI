#pragma once
#include "rnd_TextureMgr.h"
#include "rnd_SwapChainMgr.h"
#include "rnd_DescriptorHeapMgr.h"
#include "rnd_FenceMgr.h"
class core_Window;

class rnd_Dx12
{
public:
   void EnableGpuValidation();
   void OnInit(core_Window* window);
   void OnUpdate();
   void MoveToNextFrame();
   void WaitForGpu();
   void PopulateGraphicsCommandList();

   // Core window
   D3D12_VIEWPORT viewport;
   D3D12_RECT scissorRect;
   core_Window* window;

   // Device
   ComPtr<IDXGIFactory6> factory;
   ComPtr<IDXGIAdapter1> adapter;
   ComPtr<ID3D12Device> device;

   // Command list
   ComPtr<ID3D12CommandQueue> commandQueue;
   ComPtr<ID3D12CommandQueue> commandQueueCompute;
   ComPtr<ID3D12CommandQueue> commandQueueCopy;
   ComPtr<ID3D12CommandAllocator> commandAllocators[2];
   ComPtr<ID3D12CommandAllocator> commandAllocatorCompute;
   ComPtr<ID3D12CommandAllocator> commandAllocatorCopy;
   ComPtr<ID3D12GraphicsCommandList> commandList;
   ComPtr<ID3D12GraphicsCommandList> commandListCompute;
   ComPtr<ID3D12GraphicsCommandList> commandListCopy;

   // Managers
   rnd_SwapChainMgr swapChainMgr;
   rnd_DescriptorHeapMgr descriptorHeapMgr;
   rnd_TextureMgr textureMgr;
   rnd_FenceMgr fenceMgr;

   // Descriptor heaps
   ComPtr<ID3D12DescriptorHeap> dsvHeap;
   ComPtr<ID3D12DescriptorHeap> rtvHeap;
   ComPtr<ID3D12DescriptorHeap> cbvSrvUavHeap;
   ComPtr<ID3D12DescriptorHeap> svHeap; // sampler view
   int dsvSizeIncr;
   int rtvSizeIncr;
   int cbvSrvUavSizeIncr;
   int svSizeIncr;
   int dsvCount = 10;
   int rtvCount = 10;
   int cbvSrvUavCount = 300;
   int svCount = 10;

   int currentFrame = 0;
   bool rtxSupported = false;
};

