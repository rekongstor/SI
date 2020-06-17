#include <initguid.h>
#include "Dx12Renderer.h"
#include "Window.h"
#include "d3dx12.h"
#include "d3dcompiler.h"
#include <exception>
#include <iostream>
#include <algorithm>
#include <chrono>

#define SAFE_RELEASE(p) { if ( (p) ) { (p)->Release(); (p) = 0; } }
#define ASSERT(hr, msg) { if (FAILED(hr)) { std::cout << std::system_category().message(hr) << std::endl; throw std::exception(msg); }}

static Vertex vertices[] = {
   {{-0.5f, 0.5f, 0.5f}, {1.f, 0.f, 0.f}},
   {{0.5f, -0.5f, 0.5f}, {0.f, 1.f, 0.f}},
   {{-0.5f, -0.5f, 0.5f}, {0.f, 0.f, 1.f}},
   {{0.5f, 0.5f, 0.5f}, {1.f, 1.f, 1.f}},
};

static DWORD indices[] = {
   0, 1, 2,
   0, 3, 1
};

static ConstantBuffer cbColorMultiplierData;


bool Dx12Renderer::isActive() const
{
   return active;
}

void Dx12Renderer::Update()
{
   static auto clock = std::chrono::system_clock::now();
   auto clockNow = std::chrono::system_clock::now();
   auto deltaTime = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(clockNow - clock).count());
   float rIncrement = 0.00002f;
   float gIncrement = 0.00006f;
   float bIncrement = 0.00009f;

   cbColorMultiplierData.colorMultiplier.x = std::clamp(cbColorMultiplierData.colorMultiplier.x + rIncrement, 0.f, 1.f);
   cbColorMultiplierData.colorMultiplier.y = std::clamp(cbColorMultiplierData.colorMultiplier.y + gIncrement, 0.f, 1.f);
   cbColorMultiplierData.colorMultiplier.z = std::clamp(cbColorMultiplierData.colorMultiplier.z + bIncrement, 0.f, 1.f);

   cbColorMultiplierData.colorMultiplier.x = cbColorMultiplierData.colorMultiplier.x == 1.f ? 0.f : cbColorMultiplierData.colorMultiplier.x;
   cbColorMultiplierData.colorMultiplier.y = cbColorMultiplierData.colorMultiplier.y == 1.f ? 0.f : cbColorMultiplierData.colorMultiplier.y;
   cbColorMultiplierData.colorMultiplier.z = cbColorMultiplierData.colorMultiplier.z == 1.f ? 0.f : cbColorMultiplierData.colorMultiplier.z;

   memcpy(cbColorMultiplierGPUAddress[currentFrame], &cbColorMultiplierData, sizeof(cbColorMultiplierData));
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

   CD3DX12_CPU_DESCRIPTOR_HANDLE nsvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), currentFrame,
                                           rtvDescriptorSize);
   CD3DX12_CPU_DESCRIPTOR_HANDLE dsHandle(dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
   commandList->OMSetRenderTargets(1, &nsvHandle, FALSE, &dsHandle);
   commandList->ClearDepthStencilView(dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH,
                                      1.f, 0, 0, nullptr);

   const float clearColor[] = {0.f, 0.2f, 0.4f, 1.f};
   commandList->ClearRenderTargetView(nsvHandle, clearColor, 0, nullptr);

   commandList->SetGraphicsRootSignature(rootSignature);
   commandList->RSSetViewports(1, &viewport);
   commandList->RSSetScissorRects(1, &scissorRect);
   commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
   commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
   commandList->IASetIndexBuffer(&indexBufferView);

   ID3D12DescriptorHeap* descriptorHeaps[] = {mainDescriptorHeap[currentFrame]};
   commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
   commandList->SetGraphicsRootDescriptorTable(
      0, mainDescriptorHeap[currentFrame]->GetGPUDescriptorHandleForHeapStart());


   commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

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
      SAFE_RELEASE(mainDescriptorHeap[i]);
      SAFE_RELEASE(constantBufferUploadHeap[i]);
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

   // Descriptor heap, RTV
   {
      D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {
         D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
         frameBufferCount,
         D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
         0
      };
      device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));

      rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
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
      D3D12_DESCRIPTOR_RANGE descriptorRanges[] = {
         {
            D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
            1,
            0,
            0,
            D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
         }
      };

      D3D12_ROOT_DESCRIPTOR_TABLE descriptorTable = {
         _countof(descriptorRanges), descriptorRanges
      };

      D3D12_ROOT_PARAMETER rootParameters[] = {
         {D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, descriptorTable, D3D12_SHADER_VISIBILITY_VERTEX}
      };

      CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
         _countof(rootParameters),
         rootParameters,
         0,
         nullptr,
         D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
         D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
         D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
         D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
         D3D12_ROOT_SIGNATURE_FLAGS::D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS
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
                              "vs_5_0",
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
                              "ps_5_0",
                              D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
                              NULL,
                              &pixelShader,
                              &errorBlob);
      ASSERT(hr, "Failed to create pixel shader");

      D3D12_SHADER_BYTECODE vertexShaderByteCode = {vertexShader->GetBufferPointer(), vertexShader->GetBufferSize()};
      D3D12_SHADER_BYTECODE pixelShaderByteCode = {pixelShader->GetBufferPointer(), pixelShader->GetBufferSize()};

      D3D12_INPUT_ELEMENT_DESC inputElementDesc[] = {
         {
            "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
            0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
         },
         {
            "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT,
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

   // Vertex and Index buffers
   {
      // Vertex buffer

      hr = device->CreateCommittedResource(
         &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
         D3D12_HEAP_FLAG_NONE,
         &CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
         D3D12_RESOURCE_STATE_COPY_DEST,
         nullptr,
         IID_PPV_ARGS(&vertexBuffer));
      ASSERT(hr, "Failed to create vertex buffer default heap");
      vertexBuffer->SetName(L"Vertex buffer default heap");

      ID3D12Resource* vBufferUploadHeap;
      hr = device->CreateCommittedResource(
         &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
         D3D12_HEAP_FLAG_NONE,
         &CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
         D3D12_RESOURCE_STATE_GENERIC_READ,
         nullptr,
         IID_PPV_ARGS(&vBufferUploadHeap));
      ASSERT(hr, "Failed to create vertex buffer upload heap");
      vBufferUploadHeap->SetName(L"Vertex buffer upload heap");

      D3D12_SUBRESOURCE_DATA vertexData = {vertices, sizeof(vertices), sizeof(vertices)};
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
         &CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices)),
         D3D12_RESOURCE_STATE_COPY_DEST,
         nullptr,
         IID_PPV_ARGS(&indexBuffer));
      ASSERT(hr, "Failed to create index buffer default heap");
      indexBuffer->SetName(L"Index buffer default heap");

      ID3D12Resource* iBufferUploadHeap;
      hr = device->CreateCommittedResource(
         &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
         D3D12_HEAP_FLAG_NONE,
         &CD3DX12_RESOURCE_DESC::Buffer(sizeof(indices)),
         D3D12_RESOURCE_STATE_GENERIC_READ,
         nullptr,
         IID_PPV_ARGS(&iBufferUploadHeap));
      ASSERT(hr, "Failed to create index buffer upload heap");
      iBufferUploadHeap->SetName(L"Index buffer upload heap");

      D3D12_SUBRESOURCE_DATA indexData = {indices, sizeof(indices), sizeof(indices)};
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
      indexBufferView.SizeInBytes = sizeof(indices);
      indexBufferView.Format = DXGI_FORMAT_R32_UINT;

      vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
      vertexBufferView.SizeInBytes = sizeof(vertices);
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

   // CB
   {
      D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {
         D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
         1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
         0
      };
      ZeroMemory(&cbColorMultiplierData, sizeof(cbColorMultiplierData));
      for (uint32_t i = 0; i < frameBufferCount; ++i)
      {
         // Descriptor heap
         hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mainDescriptorHeap[i]));
         ASSERT(hr, "Failed to create CB descriptor heap");

         // Upload heap
         hr = device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(1024 * 64),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, IID_PPV_ARGS(&constantBufferUploadHeap[i])
         );
         ASSERT(hr, "Failed to create CB upload heap");
         constantBufferUploadHeap[i]->SetName(L"CB upload resource heap");

         // CBV

         D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc = {
            constantBufferUploadHeap[i]->GetGPUVirtualAddress(), (sizeof(ConstantBuffer) + 255) & ~255
         };
         device->CreateConstantBufferView(&constantBufferViewDesc,
                                          mainDescriptorHeap[i]->GetCPUDescriptorHandleForHeapStart());
         hr = constantBufferUploadHeap[i]->Map(
            0,
            &CD3DX12_RANGE(0, 0),
            reinterpret_cast<void**>(&cbColorMultiplierGPUAddress[i])
         );
         ASSERT(hr, "Failed mapping CB");
         memcpy(cbColorMultiplierGPUAddress[i], &cbColorMultiplierData, sizeof(cbColorMultiplierData));
      }
   }
}

void Dx12Renderer::OnUpdate()
{
   Update();
   UpdatePipeline();
   Render();
}
