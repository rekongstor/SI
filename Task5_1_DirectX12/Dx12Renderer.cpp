#include <initguid.h>
#include "Dx12Renderer.h"
#include "Window.h"
#include "d3dx12.h"
#include "d3dcompiler.h"
#include <stdexcept>
#include <iostream>
#include <chrono>
#include <wincodec.h>

#define SAFE_RELEASE(p) { if ( (p) ) { (p)->Release(); (p) = 0; } }
#define ASSERT(hr, msg) { if (FAILED(hr)) { setlocale(LC_ALL, "Russian"); std::cout << std::system_category().message(hr) << std::endl; throw std::runtime_error(msg); }}

static cbPerFrame cbPerObject;

bool Dx12Renderer::isActive() const
{
   return active;
}

void Dx12Renderer::Update()
{
   static auto timer = std::chrono::system_clock::now();
   auto timerNow = std::chrono::system_clock::now();
   float delta = static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(timerNow - timer).count()) /
      5000.f;
   timer = timerNow;

   for (auto& i : instances[&meshes[0]])
      i.rotate(DirectX::XMQuaternionRotationRollPitchYaw(0.001f * delta, 0.001f * delta, 0.001f * delta));

   XMMATRIX transformMatrix = camera.viewMatrix * camera.projMatrix;
   XMStoreFloat4x4(&cbPerObject.vpMatrix, XMMatrixTranspose(transformMatrix));
   cbPerObject.camPos = camera.position;
   memcpy(cbvGPUAddress[currentFrame], &cbPerObject, sizeof(cbPerObject));

   instanceData instanceData;
   for (size_t i = 0; i < instances[&meshes[0]].size(); ++i)
   {
      transformMatrix = instances[&meshes[0]][i].worldMatrix;
      XMStoreFloat4x4(&instanceData.wMatrix, XMMatrixTranspose(transformMatrix));
      instanceData.material = instances[&meshes[0]][i].material;
      memcpy(instanceDataGPUAddress + i * sizeof(instanceData), &instanceData, sizeof(instanceData));
   }
}

void Dx12Renderer::UpdatePipeline()
{
   HRESULT hr;

   WaitForPreviousFrame();

   hr = commandAllocator[currentFrame]->Reset();
   if (FAILED(hr))
      active = false;

   hr = commandList->Reset(commandAllocator[currentFrame], pipelineStateObject);
   if (FAILED(hr))
      active = false;

   commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
                                   renderTargets[currentFrame],
                                   D3D12_RESOURCE_STATE_PRESENT,
                                   D3D12_RESOURCE_STATE_RENDER_TARGET));

   // Drawing
   {
      CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), currentFrame,
         rtvDescriptorSize);
      CD3DX12_CPU_DESCRIPTOR_HANDLE dsHandle(dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
      commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsHandle);

      const float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.f };
      commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

      commandList->ClearDepthStencilView(dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH,
         1.f, 0, 0, nullptr);

      commandList->SetGraphicsRootSignature(rootSignature);
      commandList->RSSetViewports(1, &viewport);
      commandList->RSSetScissorRects(1, &scissorRect);
      commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
      commandList->IASetIndexBuffer(&indexBufferView);

      commandList->SetGraphicsRootConstantBufferView(0, constantBufferUploadHeaps[currentFrame]->GetGPUVirtualAddress());
      for (size_t i = 0; i < meshes.size(); ++i)
      {
         commandList->SetGraphicsRootShaderResourceView(1, instanceBuffer->GetGPUVirtualAddress());
         ID3D12DescriptorHeap* ppHeaps[] = { srvDescriptorHeap };
         commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
         commandList->SetGraphicsRootDescriptorTable(2, srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
         commandList->DrawIndexedInstanced(meshes[i].indices.size(), instances[&meshes[i]].size(), 0, 0, 0);
      }
   }

   // Postprocessing
   {
   }
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

   ID3D12CommandList* ppCommandLists[] = {commandList};
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
   SAFE_RELEASE(rtvDescriptorHeap);
   SAFE_RELEASE(commandList);

   for (uint32_t i = 0; i < frameBufferCount; ++i)
   {
      SAFE_RELEASE(renderTargets[i]);
      SAFE_RELEASE(commandAllocator[i]);
      SAFE_RELEASE(fence[i]);
   }

   SAFE_RELEASE(pipelineStateObject);
   SAFE_RELEASE(rootSignature);
   SAFE_RELEASE(vertexBuffer);
   SAFE_RELEASE(indexBuffer);

   SAFE_RELEASE(depthStencilBuffer);
   SAFE_RELEASE(dsDescriptorHeap);

   for (uint32_t i = 0; i < frameBufferCount; ++i)
   {
      SAFE_RELEASE(constantBufferUploadHeaps[i]);
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

void Dx12Renderer::LoadTexture(LPCTSTR filename, Texture& tex)
{
   HRESULT hr;
   IWICImagingFactory* wicFactory;
   CoInitialize(nullptr);
   hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory));
   ASSERT(hr, "Failed to create WIC factory");

   IWICBitmapDecoder* wicDecoder;
   hr = wicFactory->CreateDecoderFromFilename(filename, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad,
                                              &wicDecoder);
   ASSERT(hr, "Failed to create WIC decoder");

   IWICBitmapFrameDecode* wicFrame;
   hr = wicDecoder->GetFrame(0, &wicFrame);
   ASSERT(hr, "Failed to get WIC frame");

   WICPixelFormatGUID pixelFormat;
   hr = wicFrame->GetPixelFormat(&pixelFormat);
   ASSERT(hr, "Failed to get WIC frame pixel format");

   auto GetDXGIFormatFromWICFormat = [](WICPixelFormatGUID& wicFormatGUID)
   {
      if (wicFormatGUID == GUID_WICPixelFormat128bppRGBAFloat)
         return DXGI_FORMAT_R32G32B32A32_FLOAT;
      if (wicFormatGUID == GUID_WICPixelFormat64bppRGBAHalf)
         return DXGI_FORMAT_R16G16B16A16_FLOAT;
      if (wicFormatGUID == GUID_WICPixelFormat64bppRGBA)
         return DXGI_FORMAT_R16G16B16A16_UNORM;
      if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA)
         return DXGI_FORMAT_R8G8B8A8_UNORM;
      if (wicFormatGUID == GUID_WICPixelFormat32bppBGRA)
         return DXGI_FORMAT_B8G8R8A8_UNORM;
      if (wicFormatGUID == GUID_WICPixelFormat32bppBGR)
         return DXGI_FORMAT_B8G8R8X8_UNORM;
      if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102XR)
         return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;
      if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102)
         return DXGI_FORMAT_R10G10B10A2_UNORM;
      if (wicFormatGUID == GUID_WICPixelFormat16bppBGRA5551)
         return DXGI_FORMAT_B5G5R5A1_UNORM;
      if (wicFormatGUID == GUID_WICPixelFormat16bppBGR565)
         return DXGI_FORMAT_B5G6R5_UNORM;
      if (wicFormatGUID == GUID_WICPixelFormat32bppGrayFloat)
         return DXGI_FORMAT_R32_FLOAT;
      if (wicFormatGUID == GUID_WICPixelFormat16bppGrayHalf)
         return DXGI_FORMAT_R16_FLOAT;
      if (wicFormatGUID == GUID_WICPixelFormat16bppGray)
         return DXGI_FORMAT_R16_UNORM;
      if (wicFormatGUID == GUID_WICPixelFormat8bppGray)
         return DXGI_FORMAT_R8_UNORM;
      if (wicFormatGUID == GUID_WICPixelFormat8bppAlpha)
         return DXGI_FORMAT_A8_UNORM;
      return DXGI_FORMAT_UNKNOWN;
   };
   auto GetConvertToWICFormat = [](WICPixelFormatGUID& wicFormatGUID) -> WICPixelFormatGUID
   {
      if (wicFormatGUID == GUID_WICPixelFormatBlackWhite)
         return GUID_WICPixelFormat8bppGray;
      if (wicFormatGUID == GUID_WICPixelFormat1bppIndexed)
         return GUID_WICPixelFormat32bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat2bppIndexed)
         return GUID_WICPixelFormat32bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat4bppIndexed)
         return GUID_WICPixelFormat32bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat8bppIndexed)
         return GUID_WICPixelFormat32bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat2bppGray)
         return GUID_WICPixelFormat8bppGray;
      if (wicFormatGUID == GUID_WICPixelFormat4bppGray)
         return GUID_WICPixelFormat8bppGray;
      if (wicFormatGUID == GUID_WICPixelFormat16bppGrayFixedPoint)
         return GUID_WICPixelFormat16bppGrayHalf;
      if (wicFormatGUID == GUID_WICPixelFormat32bppGrayFixedPoint)
         return GUID_WICPixelFormat32bppGrayFloat;
      if (wicFormatGUID == GUID_WICPixelFormat16bppBGR555)
         return GUID_WICPixelFormat16bppBGRA5551;
      if (wicFormatGUID == GUID_WICPixelFormat32bppBGR101010)
         return GUID_WICPixelFormat32bppRGBA1010102;
      if (wicFormatGUID == GUID_WICPixelFormat24bppBGR)
         return GUID_WICPixelFormat32bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat24bppRGB)
         return GUID_WICPixelFormat32bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat32bppPBGRA)
         return GUID_WICPixelFormat32bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat32bppPRGBA)
         return GUID_WICPixelFormat32bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat48bppRGB)
         return GUID_WICPixelFormat64bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat48bppBGR)
         return GUID_WICPixelFormat64bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat64bppBGRA)
         return GUID_WICPixelFormat64bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBA)
         return GUID_WICPixelFormat64bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat64bppPBGRA)
         return GUID_WICPixelFormat64bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat48bppRGBFixedPoint)
         return GUID_WICPixelFormat64bppRGBAHalf;
      if (wicFormatGUID == GUID_WICPixelFormat48bppBGRFixedPoint)
         return GUID_WICPixelFormat64bppRGBAHalf;
      if (wicFormatGUID == GUID_WICPixelFormat64bppRGBAFixedPoint)
         return GUID_WICPixelFormat64bppRGBAHalf;
      if (wicFormatGUID == GUID_WICPixelFormat64bppBGRAFixedPoint)
         return GUID_WICPixelFormat64bppRGBAHalf;
      if (wicFormatGUID == GUID_WICPixelFormat64bppRGBFixedPoint)
         return GUID_WICPixelFormat64bppRGBAHalf;
      if (wicFormatGUID == GUID_WICPixelFormat64bppRGBHalf)
         return GUID_WICPixelFormat64bppRGBAHalf;
      if (wicFormatGUID == GUID_WICPixelFormat48bppRGBHalf)
         return GUID_WICPixelFormat64bppRGBAHalf;
      if (wicFormatGUID == GUID_WICPixelFormat128bppPRGBAFloat)
         return GUID_WICPixelFormat128bppRGBAFloat;
      if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFloat)
         return GUID_WICPixelFormat128bppRGBAFloat;
      if (wicFormatGUID == GUID_WICPixelFormat128bppRGBAFixedPoint)
         return GUID_WICPixelFormat128bppRGBAFloat;
      if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFixedPoint)
         return GUID_WICPixelFormat128bppRGBAFloat;
      if (wicFormatGUID == GUID_WICPixelFormat32bppRGBE)
         return GUID_WICPixelFormat128bppRGBAFloat;
      if (wicFormatGUID == GUID_WICPixelFormat32bppCMYK)
         return GUID_WICPixelFormat32bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat64bppCMYK)
         return GUID_WICPixelFormat64bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat40bppCMYKAlpha)
         return GUID_WICPixelFormat64bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat80bppCMYKAlpha)
         return GUID_WICPixelFormat64bppRGBA;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
      if (wicFormatGUID == GUID_WICPixelFormat32bppRGB)
         return GUID_WICPixelFormat32bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat64bppRGB)
         return GUID_WICPixelFormat64bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBAHalf)
         return GUID_WICPixelFormat64bppRGBAHalf;
#endif
      return GUID_WICPixelFormatDontCare;
   };
   auto GetDXGIFormatBitsPerPixel = [](DXGI_FORMAT& dxgiFormat)
   {
      if (dxgiFormat == DXGI_FORMAT_R32G32B32A32_FLOAT)
         return 128;
      if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_FLOAT)
         return 64;
      if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_UNORM)
         return 64;
      if (dxgiFormat == DXGI_FORMAT_R8G8B8A8_UNORM)
         return 32;
      if (dxgiFormat == DXGI_FORMAT_B8G8R8A8_UNORM)
         return 32;
      if (dxgiFormat == DXGI_FORMAT_B8G8R8X8_UNORM)
         return 32;
      if (dxgiFormat == DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM)
         return 32;
      if (dxgiFormat == DXGI_FORMAT_R10G10B10A2_UNORM)
         return 32;
      if (dxgiFormat == DXGI_FORMAT_B5G5R5A1_UNORM)
         return 16;
      if (dxgiFormat == DXGI_FORMAT_B5G6R5_UNORM)
         return 16;
      else
      {
         if (dxgiFormat == DXGI_FORMAT_R32_FLOAT)
            return 32;
         if (dxgiFormat == DXGI_FORMAT_R16_FLOAT)
            return 16;
         if (dxgiFormat == DXGI_FORMAT_R16_UNORM)
            return 16;
         if (dxgiFormat == DXGI_FORMAT_R8_UNORM)
            return 8;
         if (dxgiFormat == DXGI_FORMAT_A8_UNORM)
            return 8;
      }
   };

   auto dxgiFormat = GetDXGIFormatFromWICFormat(pixelFormat);
   bool imageConverted = false;
   IWICFormatConverter* wicFormatConverter = nullptr;
   if (dxgiFormat == DXGI_FORMAT_UNKNOWN)
   {
      auto newPixelFormat = GetConvertToWICFormat(pixelFormat);
      if (newPixelFormat == GUID_WICPixelFormatDontCare)
      {
         ASSERT(E_FAIL, "Failed to convert pixel format");
      }
      dxgiFormat = GetDXGIFormatFromWICFormat(newPixelFormat);
      hr = wicFactory->CreateFormatConverter(&wicFormatConverter);
      ASSERT(hr, "Failed to create WIC format converter");

      BOOL canConvert;
      hr = wicFormatConverter->CanConvert(pixelFormat, newPixelFormat, &canConvert);
      ASSERT(hr | canConvert ? S_OK : E_FAIL, "Failed to convert WIC to DXGI format");

      hr = wicFormatConverter->Initialize(wicFrame,
                                          newPixelFormat,
                                          WICBitmapDitherTypeErrorDiffusion,
                                          nullptr,
                                          0,
                                          WICBitmapPaletteTypeCustom);
      ASSERT(hr, "Failed to initialize WIC format converter");
      imageConverted = true;
   }
   int bitsPerPixel = GetDXGIFormatBitsPerPixel(dxgiFormat);
   UINT textureWidth, textureHeight;
   wicFrame->GetSize(&textureWidth, &textureHeight);
   int bytesPerRow = (textureWidth * bitsPerPixel) / 8;
   int imageSize = bytesPerRow * textureHeight;

   tex.pixels.resize(imageSize);
   if (imageConverted)
      wicFormatConverter->CopyPixels(nullptr, bytesPerRow, imageSize, tex.pixels.data());
   else
      wicFrame->CopyPixels(nullptr, bytesPerRow, imageSize, tex.pixels.data());

   tex.desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
   tex.desc.Alignment = 0;
   tex.desc.Width = textureWidth;
   tex.desc.Height = textureHeight;
   tex.desc.DepthOrArraySize = 1;
   tex.desc.MipLevels = 1;
   tex.desc.Format = dxgiFormat;
   tex.desc.SampleDesc.Count = 1;
   tex.desc.SampleDesc.Quality = 0;
   tex.desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
   tex.desc.Flags = D3D12_RESOURCE_FLAG_NONE;

   tex.textureHeapSize = ((((textureWidth * bitsPerPixel / 8) + 255) & ~255) * (textureHeight - 1)) + (textureWidth *
      bitsPerPixel / 8);
   tex.bytesPerRow = bytesPerRow;
}

void Dx12Renderer::OnDestroy()
{
   WaitForPreviousFrame();
   CloseHandle(fenceEvent);
   Cleanup();
}

Dx12Renderer::Dx12Renderer(Window* window, uint32_t frameBufferCount):
   window(window),
   frameBufferCount(frameBufferCount > maxFrameBufferCount ? maxFrameBufferCount : frameBufferCount),
   camera(
      {0.f, 6.0f, -1.0f, 0.f},
      {0.f, 0.f, 0.f, 0.f},
      {0.f, 1.f, 0.f, 0.f},
      45.f,
      static_cast<float>(window->getWidth()) / static_cast<float>(window->getHeight()))
{
   // Adding meshes
   {
      meshes.emplace_back(Mesh("sphere.obj"));
      for (int i = -4; i <= 4; ++i)
      {
         for (int j = -4; j <= 4; ++j)
         {
            instances[&meshes[0]].emplace_back(Instance(
               {static_cast<float>(i), 0.f, static_cast<float>(j), 0.f},
               DirectX::XMQuaternionRotationRollPitchYaw(cosf(static_cast<float>(i)), sinf(static_cast<float>(i)),
                                                         static_cast<float>(j)),
               0.20f,
               {(static_cast<float>(i) + 4.f) / 8.f, (static_cast<float>(j) + 4.f) / 8.f, 0.f, 0.f}));
         }
      }
   }
}

void Dx12Renderer::OnInit()
{
   HRESULT hr;

   // Factory
   IDXGIFactory4* factory;
   {
      hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
      ASSERT(hr, "Factory creation failed");
   }

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
      ASSERT(hr, "Adapter not found");


      hr = D3D12CreateDevice(adapter, featureLevel, IID_PPV_ARGS(&device));
      ASSERT(hr, "Device creation failed");
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
      ASSERT(hr, "Command queue creation failed");
   }

   // Swap chain
   DXGI_SAMPLE_DESC sampleDesc = {1, 0};
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
         sampleDesc,
         DXGI_USAGE_RENDER_TARGET_OUTPUT,
         (frameBufferCount),
         window->getWindow(),
         TRUE,
         (DXGI_SWAP_EFFECT_FLIP_DISCARD),
         0
      };
      IDXGISwapChain* tempSwapChain;
      hr = factory->CreateSwapChain(commandQueue, &swapChainDesc, &tempSwapChain);
      ASSERT(hr, "Swap chain creation failed");
      swapChain = static_cast<IDXGISwapChain3*>(tempSwapChain);
   }

   // Descriptor heap, RTVs
   {
      rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

      D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {
         D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
         frameBufferCount,
         D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
         0
      };
      device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));

      CD3DX12_CPU_DESCRIPTOR_HANDLE nsvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
      for (uint32_t i = 0; i < frameBufferCount; ++i)
      {
         hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
         ASSERT(hr, "Failed to get swap chain buffer");

         device->CreateRenderTargetView(renderTargets[i], nullptr, nsvHandle);
         nsvHandle.Offset(1, rtvDescriptorSize);
      }
   }


   // Depth/stencil descriptor heap, DSV
   {
      D3D12_DESCRIPTOR_HEAP_DESC dsHeapDesc = {
         (D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV),
         1,
         (D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE),
         0
      };

      hr = device->CreateDescriptorHeap(&dsHeapDesc, IID_PPV_ARGS(&dsDescriptorHeap));
      ASSERT(hr, "Failed to create descriptor heap");

      D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
      ZeroMemory(&depthStencilViewDesc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));
      depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
      depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
      depthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;

      D3D12_CLEAR_VALUE depthStencilClearValue;
      ZeroMemory(&depthStencilClearValue, sizeof(D3D12_CLEAR_VALUE));
      depthStencilClearValue.Format = DXGI_FORMAT_D32_FLOAT;
      depthStencilClearValue.DepthStencil = {1.f, 0};

      hr = device->CreateCommittedResource(
         &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
         D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
         &CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_D32_FLOAT,
            window->getWidth(),
            window->getHeight(),
            1, 0, 1, 0,
            D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
         D3D12_RESOURCE_STATE_DEPTH_WRITE,
         &depthStencilClearValue,
         IID_PPV_ARGS(&depthStencilBuffer)
      );
      ASSERT(hr, "Failed to create depth/stencil resource");
      dsDescriptorHeap->SetName(L"Depth/Stencil Resource Heap");

      device->CreateDepthStencilView(depthStencilBuffer, &depthStencilViewDesc,
                                     dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
   }

   // Command Allocator
   for (uint32_t i = 0; i < frameBufferCount; ++i)
   {
      hr = device->CreateCommandAllocator(
         D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
         IID_PPV_ARGS(&commandAllocator[i]));
      ASSERT(hr, "Failed to create command allocator");
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
      ASSERT(hr, "Failed to create command list");
   }

   // Fences
   for (uint32_t i = 0; i < frameBufferCount; ++i)
   {
      hr = device->CreateFence(
         NULL,
         D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE,
         IID_PPV_ARGS(&fence[i])
      );
      ASSERT(hr, "Failed to create fences");
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
      hr = fenceEvent ? S_OK : E_FAIL;
      ASSERT(hr, "Failed to fence event");
   }

   // Root signature
   {
      CD3DX12_ROOT_PARAMETER rootParameters[3];

      D3D12_ROOT_DESCRIPTOR rootDescriptor = {0, 0};

      rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
      rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
      rootParameters[0].Descriptor = rootDescriptor;


      rootParameters[1].InitAsShaderResourceView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);


      D3D12_DESCRIPTOR_RANGE descriptorTableRanges[1];
      descriptorTableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
      descriptorTableRanges[0].NumDescriptors = 3;
      descriptorTableRanges[0].BaseShaderRegister = 0;
      descriptorTableRanges[0].RegisterSpace = 1;
      descriptorTableRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
      D3D12_ROOT_DESCRIPTOR_TABLE descriptorTable;
      descriptorTable.NumDescriptorRanges = 1;
      descriptorTable.pDescriptorRanges = descriptorTableRanges;

      rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
      rootParameters[2].DescriptorTable = descriptorTable;
      rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;


      D3D12_STATIC_SAMPLER_DESC sampler = {
         D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_MIRROR, D3D12_TEXTURE_ADDRESS_MODE_MIRROR,
         D3D12_TEXTURE_ADDRESS_MODE_MIRROR, 0, 0, D3D12_COMPARISON_FUNC_NEVER,
         D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK, 0.f,D3D12_FLOAT32_MAX, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL
      };

      CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
         _countof(rootParameters),
         rootParameters,
         1,
         &sampler,
         D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
         D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
         D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
         D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
      );

      ID3DBlob* signature;
      hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr);
      ASSERT(hr, "Failed to serialize root signature");

      hr = device->CreateRootSignature(0,
                                       signature->GetBufferPointer(),
                                       signature->GetBufferSize(),
                                       IID_PPV_ARGS(&rootSignature));
      ASSERT(hr, "Failed to create root signature");
   }

   // Shaders, Input layout, Depth/Stencil and PSO
   {
      ID3DBlob* errorBlob;
      ID3DBlob* vertexShader;
      hr = D3DCompileFromFile(L"sample_vs.hlsl",
                              nullptr,
                              nullptr,
                              "main",
                              "vs_5_1",
                              D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
                              NULL,
                              &vertexShader,
                              &errorBlob);
      ASSERT(hr, "Failed to create vertex shader");

      ID3DBlob* pixelShader;
      hr = D3DCompileFromFile(L"sample_ps.hlsl",
                              nullptr,
                              nullptr,
                              "main",
                              "ps_5_1",
                              D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
                              NULL,
                              &pixelShader,
                              &errorBlob);
      ASSERT(hr, "Failed to create pixel shader");

      D3D12_SHADER_BYTECODE vertexShaderByteCode = {vertexShader->GetBufferPointer(), vertexShader->GetBufferSize()};
      D3D12_SHADER_BYTECODE pixelShaderByteCode = {pixelShader->GetBufferPointer(), pixelShader->GetBufferSize()};

      D3D12_INPUT_ELEMENT_DESC inputElementDesc[] = {
         {
            "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,
            0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
         },
         {
            "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,
            0,D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
         },
         {
            "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,
            0,D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
         }
      };
      D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {inputElementDesc,_countof(inputElementDesc)};


      D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
      ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
      psoDesc.InputLayout = inputLayoutDesc;
      psoDesc.pRootSignature = rootSignature;
      psoDesc.VS = vertexShaderByteCode;
      psoDesc.PS = pixelShaderByteCode;
      psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
      psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
      psoDesc.SampleDesc = sampleDesc;
      psoDesc.SampleMask = UINT_MAX;
      psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
      psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
      psoDesc.NumRenderTargets = 1;
      psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

      hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineStateObject));
      ASSERT(hr, "Failed to create PSO");
   }

   UINT srvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
   // Descriptor heaps
   {
      D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {
         D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
         3,
         D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
         0
      };
      hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&srvDescriptorHeap));
      ASSERT(hr, "Failed to create texture descriptor heap");
   }

   // Texture
   {
      CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
      auto createDXTexture = [&](LPCTSTR filename, Texture& tex)
      {
         LoadTexture(filename, tex);
         hr = device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &tex.desc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&tex.textureBuffer));
         ASSERT(hr, " Failed to create default heap for the texture");

         const UINT64 uploadBufferSize = GetRequiredIntermediateSize(tex.textureBuffer, 0, 1);

         hr = device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&tex.textureUploadHeap));
         ASSERT(hr, " Failed to create default heap for the texture");

         D3D12_SUBRESOURCE_DATA textureData = {};
         textureData.pData = tex.pixels.data();
         textureData.RowPitch = tex.bytesPerRow;
         textureData.SlicePitch = textureData.RowPitch * tex.desc.Height;

         UpdateSubresources(commandList, tex.textureBuffer, tex.textureUploadHeap, 0, 0, 1, &textureData);

         commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
                                         tex.textureBuffer, D3D12_RESOURCE_STATE_COPY_DEST,
                                         D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
         D3D12_SHADER_RESOURCE_VIEW_DESC srViewDesc = {};
         srViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
         srViewDesc.Format = tex.desc.Format;
         srViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
         srViewDesc.Texture2D.MipLevels = tex.desc.MipLevels;
         device->CreateShaderResourceView(tex.textureBuffer, &srViewDesc, srvHandle);
         srvHandle.Offset(1, srvDescriptorSize);
      };
      createDXTexture(L"albedo.dds", albedo);
      createDXTexture(L"metallic.dds", metallic);
      createDXTexture(L"roughness.dds", roughness);
   }

   // Constant buffer
   {
      for (uint32_t i = 0; i < frameBufferCount; ++i)
      {
         hr = device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(1024 * 64 * MUL_ALIGN(cbPerFrame)),
            D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&constantBufferUploadHeaps[i])
         );
         ASSERT(hr, "Failed to create upload heap for CB");
         hr = constantBufferUploadHeaps[i]->Map(0, nullptr, reinterpret_cast<void**>(&cbvGPUAddress[i]));
         ASSERT(hr, "Failed to map CB");
         cbPerObject.color = {2.f, 2.f, 2.f, 1.f};
         cbPerObject.direction = {0.f, -1.0f, 0.f, 0.f};
         cbPerObject.ambient = {0.05f, 0.05f, 0.05f, 1.f};
      }
   }

   // Instance buffer
   {
      for (uint32_t i = 0; i < frameBufferCount; ++i)
      {
         hr = device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(instances[&meshes[0]].size() * sizeof(instanceData)),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, IID_PPV_ARGS(&instanceBuffer));
         ASSERT(hr, "Failed to create instance buffer");
         hr = instanceBuffer->Map(0, nullptr, reinterpret_cast<void**>(&instanceDataGPUAddress));
      }
   }

   // Vertex and Index buffers + Closing the command list
   {
      // Vertex buffer
      hr = device->CreateCommittedResource(
         &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
         D3D12_HEAP_FLAG_NONE,
         &CD3DX12_RESOURCE_DESC::Buffer(meshes[0].vertices.size() * sizeof(Vertex)),
         D3D12_RESOURCE_STATE_COPY_DEST,
         nullptr,
         IID_PPV_ARGS(&vertexBuffer));
      ASSERT(hr, "Failed to create vertex buffer default heap");
      vertexBuffer->SetName(L"Vertex buffer default heap");

      ID3D12Resource* vBufferUploadHeap;
      hr = device->CreateCommittedResource(
         &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
         D3D12_HEAP_FLAG_NONE,
         &CD3DX12_RESOURCE_DESC::Buffer(meshes[0].vertices.size() * sizeof(Vertex)),
         D3D12_RESOURCE_STATE_GENERIC_READ,
         nullptr,
         IID_PPV_ARGS(&vBufferUploadHeap));
      ASSERT(hr, "Failed to create vertex buffer upload heap");
      vBufferUploadHeap->SetName(L"Vertex buffer upload heap");

      D3D12_SUBRESOURCE_DATA vertexData = {
         meshes[0].vertices.data(), meshes[0].vertices.size() * sizeof(Vertex),
         meshes[0].vertices.size() * sizeof(Vertex)
      };
      UpdateSubresources(commandList, vertexBuffer, vBufferUploadHeap, 0, 0, 1, &vertexData);

      commandList->ResourceBarrier(1,
                                   &CD3DX12_RESOURCE_BARRIER::Transition(
                                      vertexBuffer,
                                      D3D12_RESOURCE_STATE_COPY_DEST,
                                      D3D12_RESOURCE_STATE_INDEX_BUFFER));

      // Index buffer


      hr = device->CreateCommittedResource(
         &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
         D3D12_HEAP_FLAG_NONE,
         &CD3DX12_RESOURCE_DESC::Buffer(meshes[0].indices.size() * sizeof(DWORD)),
         D3D12_RESOURCE_STATE_COPY_DEST,
         nullptr,
         IID_PPV_ARGS(&indexBuffer));
      ASSERT(hr, "Failed to create index buffer default heap");
      indexBuffer->SetName(L"Index buffer default heap");

      ID3D12Resource* iBufferUploadHeap;
      hr = device->CreateCommittedResource(
         &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
         D3D12_HEAP_FLAG_NONE,
         &CD3DX12_RESOURCE_DESC::Buffer(meshes[0].indices.size() * sizeof(DWORD)),
         D3D12_RESOURCE_STATE_GENERIC_READ,
         nullptr,
         IID_PPV_ARGS(&iBufferUploadHeap));
      ASSERT(hr, "Failed to create index buffer upload heap");
      iBufferUploadHeap->SetName(L"Index buffer upload heap");

      D3D12_SUBRESOURCE_DATA indexData = {
         meshes[0].indices.data(), meshes[0].indices.size() * sizeof(DWORD), meshes[0].indices.size() * sizeof(DWORD)
      };
      UpdateSubresources(commandList, indexBuffer, iBufferUploadHeap, 0, 0, 1, &indexData);

      commandList->ResourceBarrier(1,
                                   &CD3DX12_RESOURCE_BARRIER::Transition(
                                      indexBuffer,
                                      D3D12_RESOURCE_STATE_COPY_DEST,
                                      D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
      commandList->Close();
      ID3D12CommandList* ppCommandLists[] = {commandList};
      commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

      ++fenceValue[currentFrame];
      hr = commandQueue->Signal(fence[currentFrame], fenceValue[currentFrame]);
      if (FAILED(hr))
         active = false;

      indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
      indexBufferView.SizeInBytes = meshes[0].indices.size() * sizeof(DWORD);
      indexBufferView.Format = DXGI_FORMAT_R32_UINT;

      vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
      vertexBufferView.SizeInBytes = meshes[0].vertices.size() * sizeof(Vertex);
      vertexBufferView.StrideInBytes = sizeof(Vertex);
   }

   // Viewport
   {
      viewport.TopLeftX = 0;
      viewport.TopLeftY = 0;
      viewport.Width = window->getWidth();
      viewport.Height = window->getHeight();
      viewport.MinDepth = 0.f;
      viewport.MaxDepth = 1.f;

      scissorRect.left = 0;
      scissorRect.right = 0;
      scissorRect.right = window->getWidth();
      scissorRect.bottom = window->getHeight();
   }
}

void Dx12Renderer::OnUpdate()
{
   Update();
   UpdatePipeline();
   Render();
}
