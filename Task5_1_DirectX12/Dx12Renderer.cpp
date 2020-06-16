#include "Dx12Renderer.h"
#include "Window.h"
#include "d3dx12.h"
#include <exception>
#include <iostream>

#define SAFE_RELEASE(p) { if ( (p) ) { (p)->Release(); (p) = 0; } }

bool Dx12Renderer::isActive() const
{
   return active;
}

void Dx12Renderer::Update()
{
}

void Dx12Renderer::UpdatePipeline()
{
   HRESULT hr;

   WaitForPreviousFrame();

   hr = commandAllocator[currentFrame]->Reset();
   if (FAILED(hr))
      active = false;

   hr = commandList->Reset(commandAllocator[currentFrame], nullptr);
   if (FAILED(hr))
      active = false;

   commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
                                   renderTargets[currentFrame],
                                   D3D12_RESOURCE_STATE_PRESENT,
                                   D3D12_RESOURCE_STATE_RENDER_TARGET));

   CD3DX12_CPU_DESCRIPTOR_HANDLE nsvHandle(nsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), currentFrame,
                                           rtvDescriptorSize);

   commandList->OMSetRenderTargets(1, &nsvHandle, FALSE, nullptr);

   const float clearColor[] = {0.f, 0.2f, 0.4f, 1.f};
   commandList->ClearRenderTargetView(nsvHandle, clearColor, 0, nullptr);

   commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
      renderTargets[currentFrame],
      D3D12_RESOURCE_STATE_RENDER_TARGET,
      D3D12_RESOURCE_STATE_PRESENT));

   hr = commandList->Close();
   if (FAILED(hr))
      active = false;
}

void Dx12Renderer::Render()
{
   HRESULT hr;

   ID3D12CommandList* ppCommandLists[] = { commandList };
   commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

   hr = commandQueue->Signal(fence[currentFrame], fenceValue[currentFrame]);
   if (FAILED(hr))
      active = false;

   hr = swapChain->Present(0, 0);
   if (FAILED(hr))
      active = false;
}

void Dx12Renderer::Cleanup()
{
   for (uint32_t i = 0; i < frameBufferCount; ++i)
   {
      currentFrame = i;
      WaitForPreviousFrame();
   }

   BOOL fs = false;
   if (swapChain->GetFullscreenState(&fs, nullptr))
      swapChain->SetFullscreenState(false, nullptr);

   SAFE_RELEASE(device);
   SAFE_RELEASE(swapChain);
   SAFE_RELEASE(commandQueue);
   SAFE_RELEASE(nsvDescriptorHeap);
   SAFE_RELEASE(commandList);

   for (uint32_t i = 0; i < frameBufferCount; ++i)
   {
      SAFE_RELEASE(renderTargets[i]);
      SAFE_RELEASE(commandAllocator[i]);
      SAFE_RELEASE(fence[i]);
   }
}

void Dx12Renderer::WaitForPreviousFrame()
{
   HRESULT hr;

   currentFrame = swapChain->GetCurrentBackBufferIndex();
   if (fence[currentFrame]->GetCompletedValue() < fenceValue[currentFrame])
   {
      hr = fence[currentFrame]->SetEventOnCompletion(fenceValue[currentFrame], fenceEvent);
      if (FAILED(hr))
         active = false;
      WaitForSingleObject(fenceEvent, INFINITE);
   }

   ++fenceValue[currentFrame];
}

void Dx12Renderer::OnDestroy()
{
   WaitForPreviousFrame();
   CloseHandle(fenceEvent);
   Cleanup();
}

Dx12Renderer::Dx12Renderer(Window* window, uint32_t frameBufferCount): window(window),
                                                                       frameBufferCount(
                                                                          frameBufferCount > maxFrameBufferCount
                                                                             ? maxFrameBufferCount
                                                                             : frameBufferCount)
{
}

void Dx12Renderer::OnInit()
{
   HRESULT hr;

   // Factory
   IDXGIFactory4* factory;
   hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
   if (FAILED(hr))
      throw std::exception("Factory creation failed");

   // Adapter
   {
      IDXGIAdapter1* adapter = nullptr;
      for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex
      )
      {
         DXGI_ADAPTER_DESC1 desc;
         hr = adapter->GetDesc1(&desc);

         if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;
         if (SUCCEEDED(D3D12CreateDevice(adapter, featureLevel, __uuidof(ID3D12Device), nullptr)))
            break;
      }
      if (adapter == nullptr)
         throw std::exception("Adapter not found");


      hr = D3D12CreateDevice(adapter, featureLevel, IID_PPV_ARGS(&device));
      if (FAILED(hr))
         throw std::exception("Device creation failed");
   }

   // Command Queue
   {
      D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {
         D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
         D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
         D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE,
         0
      };
      hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));
      if (FAILED(hr))
         throw std::exception("Command queue creation failed");
   }

   // Swap chain
   {
      DXGI_SWAP_CHAIN_DESC swapChainDesc = {
         {
            window->getWidth(),
            window->getHeight(),
            {0, 0},
            (DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM),
            (DXGI_MODE_SCANLINE_ORDER::DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED),
            (DXGI_MODE_SCALING::DXGI_MODE_SCALING_UNSPECIFIED)
         },
         {1, 0},
         DXGI_USAGE_RENDER_TARGET_OUTPUT,
         (frameBufferCount),
         window->getWindow(),
         TRUE,
         (DXGI_SWAP_EFFECT_FLIP_DISCARD),
         0
      };
      IDXGISwapChain* tempSwapChain;
      hr = factory->CreateSwapChain(commandQueue, &swapChainDesc, &tempSwapChain);
      if (FAILED(hr))
         throw std::exception("Swap chain creation failed");
      swapChain = static_cast<IDXGISwapChain3*>(tempSwapChain);
   }

   // Descriptor heap, RTV
   {
      D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {
         D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
         frameBufferCount,
         D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
         0
      };
      device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&nsvDescriptorHeap));

      rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
      CD3DX12_CPU_DESCRIPTOR_HANDLE nsvHandle(nsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
      for (uint32_t i = 0; i < frameBufferCount; ++i)
      {
         hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
         if (FAILED(hr))
            throw std::exception("Failed to get swap chain buffer");

         device->CreateRenderTargetView(renderTargets[i], nullptr, nsvHandle);
         nsvHandle.Offset(1, rtvDescriptorSize);
      }
   }

   // Command Allocator
   for (uint32_t i = 0; i < frameBufferCount; ++i)
   {
      hr = device->CreateCommandAllocator(
         D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
         IID_PPV_ARGS(&commandAllocator[i]));
      if (FAILED(hr))
         throw std::exception("Failed to create command allocator");
   }

   // Command list
   {
      hr = device->CreateCommandList(
         0,
         D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
         commandAllocator[0],
         nullptr,
         IID_PPV_ARGS(&commandList)
      );
      if (FAILED(hr))
         throw std::exception("Failed to create command list");
      commandList->Close();
   }

   // Fences
   for (uint32_t i = 0; i < frameBufferCount; ++i)
   {
      hr = device->CreateFence(
         NULL,
         D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE,
         IID_PPV_ARGS(&fence[i])
      );
      if (FAILED(hr))
         throw std::exception("Failed to create fences");
      fenceValue[i] = 0;
   }

   // Fence event
   {
      fenceEvent = CreateEvent(
         nullptr,
         FALSE,
         FALSE,
         nullptr
      );
      if (fenceEvent == nullptr)
         throw std::exception("Failed to fence event");
   }
}

void Dx12Renderer::OnUpdate()
{
   Update();
   UpdatePipeline();
   Render();
}
