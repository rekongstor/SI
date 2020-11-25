#include "rnd_Dx12.h"
#include "core_Window.h"

void rnd_Dx12::EnableGpuValidation()
{
   ComPtr<ID3D12Debug> debugController;
   ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
   debugController->EnableDebugLayer();

   ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
   ThrowIfFailed(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue)));
   ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory)));

   dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
   dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
}


inline bool IsDirectXRaytracingSupported(IDXGIAdapter1* adapter)
{
   ComPtr<ID3D12Device> testDevice;
   D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupportData {};

   return SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&testDevice)))
      && SUCCEEDED(testDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupportData, sizeof(featureSupportData)))
      && featureSupportData.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
}


void rnd_Dx12::OnInit(core_Window* window)
{
#pragma region GPU_Validation
#ifdef GPU_VALIDATION
   EnableGpuValidation();
#endif
#pragma endregion

#pragma region Window
   this->window = window;
   viewport.Width = window->width;
   viewport.Height = window->height;
   viewport.TopLeftX = viewport.TopLeftY = 0.f;
   viewport.MaxDepth = 1.f;
   viewport.MinDepth = 0.f;

   scissorRect.left = scissorRect.top = 0.f;
   scissorRect.right = window->width;
   scissorRect.bottom = window->height;
#pragma endregion

#pragma region Factory
#ifndef GPU_VALIDATION
   ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
#endif
#pragma endregion

#pragma region Device
   D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
   ComPtr<IDXGIAdapter1> adapterTmp;
   DXGI_ADAPTER_DESC1 desc;
   for (UINT adapterID = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterID, &adapterTmp); ++adapterID)
   {
      ThrowIfFailed(adapterTmp->GetDesc1(&desc));

      if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
      {
         continue;
      }

      if (SUCCEEDED(D3D12CreateDevice(adapterTmp.Get(), featureLevel, __uuidof(ID3D12Device), nullptr)))
      {
         break;
      }
   }
   IDXGIAdapter1** ppAdapter = &adapter;
   *ppAdapter = adapterTmp.Detach();
   rtxSupported = IsDirectXRaytracingSupported(adapter.Get());

   ThrowIfFailed(D3D12CreateDevice(adapter.Get(), featureLevel, IID_PPV_ARGS(&device)));


   device.Get()->SetName(L"Device");
#pragma endregion

   // TODO: PSOs here
#pragma region PSO

#pragma endregion 

   // TODO: Add bundles?
#pragma region CommandQueues 
   D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {
      D3D12_COMMAND_LIST_TYPE_DIRECT,
      D3D12_COMMAND_QUEUE_PRIORITY_NORMAL, // set higher priority, could be failed
      D3D12_COMMAND_QUEUE_FLAG_NONE, // disable GPU timeout
      0 // multi-GPU
   };
   ThrowIfFailed(device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue)));
   commandQueue->SetName(L"Command queue direct");
   commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
   ThrowIfFailed(device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueueCompute)));
   commandQueueCompute->SetName(L"Command queue compute");
   commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
   ThrowIfFailed(device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueueCopy)));
   commandQueueCopy->SetName(L"Command queue copy");
  
#pragma endregion 

#pragma region CommandAllocators
   ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[0])));
   commandAllocators[0]->SetName(L"COMMAND ALLOCATOR 0: Direct");
   ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[1])));
   commandAllocators[1]->SetName(L"COMMAND ALLOCATOR 1: Direct");

   ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&commandAllocatorCompute)));
   commandAllocatorCompute->SetName(L"COMMAND ALLOCATOR: Compute");
   ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&commandAllocatorCopy)));
   commandAllocatorCopy->SetName(L"COMMAND ALLOCATOR: Copy");
#pragma endregion 

#pragma region CommandList
   ThrowIfFailed(device->CreateCommandList(        /*multi-gpu*/ 0, 
      D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[0].Get(), /* Initial PSO */ nullptr, 
      IID_PPV_ARGS(&commandList)));
   commandList->SetName(L"Command List Direct");

   ThrowIfFailed(device->CreateCommandList(        /*multi-gpu*/ 0,
      D3D12_COMMAND_LIST_TYPE_COMPUTE, commandAllocatorCompute.Get(), /* Initial PSO */ nullptr,
      IID_PPV_ARGS(&commandListCompute)));
   commandListCompute->SetName(L"Command List Compute");

   ThrowIfFailed(device->CreateCommandList(        /*multi-gpu*/ 0,
      D3D12_COMMAND_LIST_TYPE_COPY, commandAllocatorCopy.Get(), /* Initial PSO */ nullptr,
      IID_PPV_ARGS(&commandListCopy)));
   commandListCopy->SetName(L"Command List Copy");
#pragma endregion 

   swapChainMgr.OnInit(factory.Get(), commandQueue.Get(), &textureMgr, window, DXGI_FORMAT_R8G8B8A8_UNORM, true);
   fenceMgr.OnInit(device.Get());
   descriptorHeapMgr;
}



void rnd_Dx12::OnUpdate()
{
   PopulateGraphicsCommandList();

   MoveToNextFrame();
}

void rnd_Dx12::MoveToNextFrame()
{
   ThrowIfFailed(commandList->Close());
   ID3D12CommandList* cmdLists = { commandList.Get() };
   commandQueue->ExecuteCommandLists(1, &cmdLists);

   swapChainMgr.swapChain->Present(0, 0);

   fenceMgr.MoveToNextFrame(commandQueue.Get(), swapChainMgr.swapChain.Get(), swapChainMgr.currentFrame);
}

void rnd_Dx12::WaitForGpu()
{
   ThrowIfFailed(commandList->Close());

   ID3D12CommandList* cmdLists = { commandList.Get() };
   commandQueue->ExecuteCommandLists(1, &cmdLists);

   fenceMgr.WaitForGpu(commandQueue.Get());
}

void rnd_Dx12::PopulateGraphicsCommandList()
{
   // TODO: Make implementation
}
