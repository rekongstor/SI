#include "rnd_Dx12.h"
#include "core_Window.h"

// Managers
rnd_SwapChainMgr swapChainMgr;
rnd_DescriptorHeapMgr descriptorHeapMgr;
rnd_TextureMgr textureMgr;
rnd_CommandMgr commandMgr;

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
   D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_12_1;
   ComPtr<IDXGIAdapter1> adapterTmp;
   DXGI_ADAPTER_DESC1 desc;
   for (UINT adapterId = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterId, &adapterTmp); ++adapterId)
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

#pragma region Fences
   ThrowIfFailed(device->CreateFence(fenceValues[currentFrame], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
   ++fenceValues[currentFrame];
   fence->SetName(L"Fence");

   fenceEvent.Attach(CreateEvent(nullptr, FALSE, FALSE, nullptr));
   assert(fenceEvent.IsValid());
#pragma endregion 

#pragma region Managers
   commandMgr.OnInit(device.Get());
   descriptorHeapMgr.OnInit(device.Get());
   swapChainMgr.OnInit(factory.Get(), commandMgr.commandQueue.Get(), device.Get(), &textureMgr, &descriptorHeapMgr, window, DXGI_FORMAT_R8G8B8A8_UNORM, windowed);
   rayTracingPipeline.OnInit(this);
#pragma endregion 

   WaitForGpu();
}

void rnd_Dx12::OnUpdate()
{
   PopulateGraphicsCommandList();

   MoveToNextFrame();
}

void rnd_Dx12::MoveToNextFrame()
{
   commandMgr.SetBarrier({ {textureMgr.backBuffer[currentFrame], D3D12_RESOURCE_STATE_PRESENT} });

   ExecuteCommandList();

   if (tearingSupported) {
      ThrowIfFailed(swapChainMgr.swapChain->Present(syncInterval != UINT_MAX ? syncInterval : 0, DXGI_PRESENT_ALLOW_TEARING));
   } else {
      ThrowIfFailed(swapChainMgr.swapChain->Present(syncInterval != UINT_MAX ? syncInterval : 1, 0));
   }

   const UINT64 currentFenceValue = fenceValues[currentFrame];
   ThrowIfFailed(commandMgr.commandQueue->Signal(fence.Get(), currentFenceValue));

   currentFrame = swapChainMgr.swapChain->GetCurrentBackBufferIndex();

   if (fence->GetCompletedValue() < fenceValues[currentFrame]) {
      ThrowIfFailed(fence->SetEventOnCompletion(fenceValues[currentFrame], fenceEvent.Get()));
      WaitForSingleObjectEx(fenceEvent.Get(), INFINITE, FALSE);
   }

   fenceValues[currentFrame] = currentFenceValue + 1;
}

void rnd_Dx12::ExecuteCommandList()
{
   ThrowIfFailed(commandMgr.commandList->Close());
   ID3D12CommandList* commandLists[] = { commandMgr.Get() };
   commandMgr.commandQueue->ExecuteCommandLists(ARRAYSIZE(commandLists), commandLists);
}

void rnd_Dx12::WaitForGpu()
{
   if (commandMgr.commandQueue && fence && fenceEvent.IsValid()) {
      UINT64 fenceValue = fenceValues[currentFrame];
      if (SUCCEEDED(commandMgr.commandQueue->Signal(fence.Get(), fenceValue))) {
         // Wait until the Signal has been processed.
         if (SUCCEEDED(fence->SetEventOnCompletion(fenceValue, fenceEvent.Get()))) {
            WaitForSingleObjectEx(fenceEvent.Get(), INFINITE, FALSE);

            fenceValues[currentFrame]++;
         }
      }
   }
}

void rnd_Dx12::PopulateGraphicsCommandList()
{
   ThrowIfFailed(commandMgr.commandAllocators[currentFrame]->Reset());
   ThrowIfFailed(commandMgr.commandList->Reset(commandMgr.commandAllocators[currentFrame].Get(), nullptr)); // TODO: initial pipeline state!

   commandMgr.SetBarrier({ {textureMgr.backBuffer[currentFrame], D3D12_RESOURCE_STATE_RENDER_TARGET} });

   FLOAT color[]{ 0.1,0.2,0.3,0.4 };
   commandMgr.commandList->ClearRenderTargetView(textureMgr.backBuffer[currentFrame].rtvHandle.first, color, 1, &scissorRect);

   rayTracingPipeline.DoRaytracing();
   rayTracingPipeline.CopyRaytracingOutputToBackbuffer();
}
