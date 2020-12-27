#include "rnd_Dx12.h"
#include "core_Window.h"
#include "core_Imgui.h"
#include "HlslCompat.h"

wchar_t nameBuffer[4096]{};

#pragma region RT helpers

inline void AllocateUploadBuffer(ID3D12Device* pDevice, void* pData, UINT64 datasize, ID3D12Resource** ppResource, const wchar_t* resourceName = nullptr)
{
   auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
   auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(datasize);
   ThrowIfFailed(pDevice->CreateCommittedResource(
      &uploadHeapProperties,
      D3D12_HEAP_FLAG_NONE,
      &bufferDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(ppResource)));
   if (resourceName) {
      (*ppResource)->SetName(resourceName);
   }
   void* pMappedData;
   (*ppResource)->Map(0, nullptr, &pMappedData);
   memcpy(pMappedData, pData, datasize);
   (*ppResource)->Unmap(0, nullptr);
}

inline void AllocateUAVBuffer(ID3D12Device* pDevice, UINT64 bufferSize, ID3D12Resource** ppResource, D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_COMMON, const wchar_t* resourceName = nullptr)
{
   auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
   auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
   ThrowIfFailed(pDevice->CreateCommittedResource(
      &uploadHeapProperties,
      D3D12_HEAP_FLAG_NONE,
      &bufferDesc,
      initialResourceState,
      nullptr,
      IID_PPV_ARGS(ppResource)));
   if (resourceName) {
      (*ppResource)->SetName(resourceName);
   }
}

#pragma endregion

void rnd_Dx12::OnInit()
{
#pragma region Factory
#ifdef GPU_VALIDATION
   ComPtr<ID3D12Debug> debugController;
   ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
   debugController->EnableDebugLayer();

   ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
   ThrowIfFailed(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue)));
   ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory)));

   dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
   dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
#else
   ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
#endif
#pragma endregion

#pragma region Window
   viewport.Width = (float)window->width;
   viewport.Height = (float)window->height;
   viewport.TopLeftX = viewport.TopLeftY = 0.f;
   viewport.MaxDepth = 1.f;
   viewport.MinDepth = 0.f;

   scissorRect.left = scissorRect.top = 0;
   scissorRect.right = window->width;
   scissorRect.bottom = window->height;
#pragma endregion

#pragma region Device
   D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
   ComPtr<IDXGIAdapter1> adapterTmp;
   DXGI_ADAPTER_DESC1 desc;
   for (UINT adapterId = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterId, &adapterTmp); ++adapterId) {
      ThrowIfFailed(adapterTmp->GetDesc1(&desc));

      if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
         continue;
      }

      if (SUCCEEDED(D3D12CreateDevice(adapterTmp.Get(), featureLevel, __uuidof(ID3D12Device), nullptr))) {
         break;
      }
   }
   IDXGIAdapter1** ppAdapter = &adapter;
   *ppAdapter = adapterTmp.Detach();
   ComPtr<ID3D12Device> testDevice;
   D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupportData{};

   rtxSupported = SUCCEEDED(D3D12CreateDevice(adapter.Get(), featureLevel, IID_PPV_ARGS(&testDevice)))
      && SUCCEEDED(testDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupportData, sizeof(featureSupportData)))
      && featureSupportData.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;

   ThrowIfFailed(D3D12CreateDevice(adapter.Get(), featureLevel, IID_PPV_ARGS(&device)));
   device.Get()->SetName(L"Device");
#pragma endregion

#pragma region Fences
   ThrowIfFailed(device->CreateFence(fenceValues[currentFrame], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
   ThrowIfFailed(device->CreateFence(fenceValueCopy, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fenceCopy)));
   ++fenceValues[currentFrame];
   ++fenceValueCopy;
   fence->SetName(L"Fence");
   fenceCopy->SetName(L"FenceCopy");

   fenceEvent.Attach(CreateEvent(nullptr, FALSE, FALSE, nullptr));
   assert(fenceEvent.IsValid());
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
   for (int i = 0; i < FRAME_COUNT; ++i)
   {
      LPCWSTR name = FormatWStr(L"COMMAND ALLOCATOR %d: Direct", i);
      ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[i])));
      commandAllocators[i]->SetName(name); 
   }

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
   commandList->Close();

   ThrowIfFailed(device->CreateCommandList(        /*multi-gpu*/ 0,
      D3D12_COMMAND_LIST_TYPE_COMPUTE, commandAllocatorCompute.Get(), /* Initial PSO */ nullptr,
      IID_PPV_ARGS(&commandListCompute)));
   commandListCompute->SetName(L"Command List Compute");
   commandListCompute->Close();

   ThrowIfFailed(device->CreateCommandList(        /*multi-gpu*/ 0,
      D3D12_COMMAND_LIST_TYPE_COPY, commandAllocatorCopy.Get(), /* Initial PSO */ nullptr,
      IID_PPV_ARGS(&commandListCopy)));
   commandListCopy->SetName(L"Command List Copy");
   //commandListCopy->Close();
#pragma endregion 

#pragma region Descriptor heap
   auto heapInit = [](ID3D12Device* device, ComPtr<ID3D12DescriptorHeap>& heap, CD3DX12_CPU_DESCRIPTOR_HANDLE& cpuHandle,
      CD3DX12_GPU_DESCRIPTOR_HANDLE& gpuHandle, D3D12_DESCRIPTOR_HEAP_TYPE heapType, D3D12_DESCRIPTOR_HEAP_FLAGS flags, uint32_t heapSize) {
         D3D12_DESCRIPTOR_HEAP_DESC desc({ heapType, heapSize, flags, 0 });
         ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));
         cpuHandle = heap->GetCPUDescriptorHandleForHeapStart();
         gpuHandle = heap->GetGPUDescriptorHandleForHeapStart();
   };

   heapInit(device.Get(), dsvHeap, dsvHandle.first, dsvHandle.second,
      D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, dsvCount);
   dsvHeap.Get()->SetName(L"Descriptor heap DSV");

   heapInit(device.Get(), rtvHeap, rtvHandle.first, rtvHandle.second,
      D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, rtvCount);
   dsvHeap.Get()->SetName(L"Descriptor heap RTV");

   heapInit(device.Get(), cbvSrvUavHeap, cbvSrvUavHandle.first, cbvSrvUavHandle.second,
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, cbvSrvUavCount);
   dsvHeap.Get()->SetName(L"Descriptor heap SrvUav");

   heapInit(device.Get(), svHeap, samplerHandle.first, samplerHandle.second,
      D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, svCount);
   dsvHeap.Get()->SetName(L"Descriptor heap Sampler");

   dsvIncrSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
   rtvIncrSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
   cbvSrvUavIncrSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
   svIncrSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
#pragma endregion 

#pragma region Swap chain
   assert(window != nullptr);
   assert(window->width > 0 && window->height > 0);

   DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
   swapChainDesc.Width = window->width;
   swapChainDesc.Height = window->height;
   swapChainDesc.Format = swapChainFormat;
   swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // TODO: Do I need this? Maybe I should make this UAV for DLSS?
   swapChainDesc.BufferCount = FRAME_COUNT;
   swapChainDesc.SampleDesc.Count = 1;
   swapChainDesc.SampleDesc.Quality = 0;
   swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
   swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
   swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
   swapChainDesc.Flags = 0; // enable swapChain.Resize
   // TODO: DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING?

   DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsDesc{};
   fsDesc.Windowed = windowed;
   window->fullscreen = !windowed;

   ComPtr<IDXGISwapChain1> swapChainTmp;
   bool prevIsFullscreen = window->IsFullscreen();
   if (prevIsFullscreen) {
      window->SetWindowZorderToTopMost(false);
   }

   ThrowIfFailed(factory->CreateSwapChainForHwnd(commandQueue.Get(), window->window, &swapChainDesc, &fsDesc, nullptr, &swapChainTmp));

   if (prevIsFullscreen) {
      window->SetWindowZorderToTopMost(true);
   }

   ThrowIfFailed(swapChainTmp.As(&swapChain));

   ID3D12Resource* backBuffer;
   for (int i = 0; i < FRAME_COUNT; ++i)
   {
      LPCWSTR name = FormatWStr(L"BackBuffer_%d", i);
      ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));
      textureMgr.backBuffer[i].OnInit(backBuffer, D3D12_RESOURCE_STATE_PRESENT, name);
      textureMgr.backBuffer[i].CreateRtv();
   }
#pragma endregion

   camPos = { 2, 2, 6, 0 };
   camDir = { -0.25, -0.3 };
   lightPosition = { 0, 1, -1, 0 };
   lightAmbientColor = { 0.5, 0.0, 0.5, 0 };
   lightDiffuseColor = { 0.0, 0.5, 0.5, 0 };
   lightDirection = { 1, 1, 0.5, 0 };
   fovAngleY = 60.f;

   constantBufferMgr.InitConstBuffers();
   scene.OnInit("data/scenes/rtdlgi.fbx");
   ResolveUploadBuffer();

   forwardPass.OnInit();

   rtxPass.OnInit();
   InitRaytracing();

   textureMgr.depthBuffer.OnInit(DXGI_FORMAT_D32_FLOAT, { window->width, window->height }, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, D3D12_RESOURCE_STATE_DEPTH_WRITE, L"DEPTH_BUFFER");
   textureMgr.depthBuffer.CreateDsv();

   if (imgui)
      imgui->InitRender();

   WaitForGpu();
}

void rnd_Dx12::PopulateGraphicsCommandList()
{
   ThrowIfFailed(commandAllocators[currentFrame]->Reset());
   ThrowIfFailed(commandList->Reset(commandAllocators[currentFrame].Get(), nullptr)); // TODO: initial pipeline state!

   SetBarrier({ {textureMgr.backBuffer[currentFrame], D3D12_RESOURCE_STATE_RENDER_TARGET} });

   FLOAT color[]{ 0.1f, 0.2f, 0.3f, 0.4f };
   commandList->ClearRenderTargetView(textureMgr.backBuffer[currentFrame].rtvHandle.first, color, 1, &scissorRect);
      
   commandList->ClearDepthStencilView(textureMgr.depthBuffer.dsvHandle.first, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 1, &scissorRect);

   DoRaytracing();
   CopyRaytracingOutputToBackbuffer();

   forwardPass.Execute();

   commandList->OMSetRenderTargets(1,
      &textureMgr.backBuffer[currentFrame].rtvHandle.first,
      FALSE,
      nullptr);
   if (imgui)
      imgui->OnRender();
}

#pragma region RT
void rnd_Dx12::InitRaytracing()
{
   // Create raytracing interfaces: raytracing device and commandlist.
   ThrowIfFailed(device->QueryInterface(IID_PPV_ARGS(&dxrDevice)), L"Couldn't get DirectX Raytracing interface for the device.\n");
   ThrowIfFailed(commandList->QueryInterface(IID_PPV_ARGS(&dxrCommandList)), L"Couldn't get DirectX Raytracing interface for the command list.\n");

   // Create root signatures for the shaders.
   m_raytracingGlobalRootSignature = rootSignatureMgr.CreateRootSignature({
      DescTable({
         DescRange(RngType::UAV, 1, 0)
      }),
      SRV(0),
      CBV(0),
      DescTable({
         DescRange(RngType::SRV, 2, 1)
      })
      });

   rtxPass.CreateRootSignature();

   rtxPass.CreateRaytracingPipelineStateObject();

   // Build raytracing acceleration structures from the generated geometry.
   BuildAccelerationStructures();

   rtxPass.BuildShaderTables();
   // Create an output 2D texture to store the raytracing result to.
   CreateRaytracingOutputResource();
}

void rnd_Dx12::DoRaytracing()
{
   rtxPass.Execute();
}

void rnd_Dx12::CopyRaytracingOutputToBackbuffer()
{
   auto& renderTarget = textureMgr.backBuffer[currentFrame];
   auto& rayTracingOutput = textureMgr.rayTracingOutput;

   SetBarrier({ {renderTarget, D3D12_RESOURCE_STATE_COPY_DEST}, {rayTracingOutput, D3D12_RESOURCE_STATE_COPY_SOURCE} });

   commandList->CopyResource(renderTarget.buffer.Get(), rayTracingOutput.buffer.Get());

   SetBarrier({ {renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET}, {rayTracingOutput, D3D12_RESOURCE_STATE_UNORDERED_ACCESS} }); // TODO: Do I need this?
}

void rnd_Dx12::BuildAccelerationStructures()
{
   auto commandAllocator = commandAllocators[currentFrame].Get();

   // Reset the command list for the acceleration structure construction.
   commandList->Reset(commandAllocator, nullptr);

   D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
   geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
   geometryDesc.Triangles.IndexBuffer = renderer->scene.meshes[0].indexBuffer.buffer->GetGPUVirtualAddress();
   geometryDesc.Triangles.IndexCount = static_cast<UINT>(renderer->scene.meshes[0].indexBuffer.buffer->GetDesc().Width) / sizeof(Index);
   geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
   geometryDesc.Triangles.Transform3x4 = 0;
   geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
   geometryDesc.Triangles.VertexCount = static_cast<UINT>(renderer->scene.meshes[0].vertexBuffer.buffer->GetDesc().Width) / sizeof(Vertex);
   geometryDesc.Triangles.VertexBuffer.StartAddress = renderer->scene.meshes[0].vertexBuffer.buffer->GetGPUVirtualAddress();
   geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);

   // Mark the geometry as opaque. 
   // PERFORMANCE TIP: mark geometry as opaque whenever applicable as it can enable important ray processing optimizations.
   // Note: When rays encounter opaque geometry an any hit shader will not be executed whether it is present or not.
   geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

   // Get required sizes for an acceleration structure.
   D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

   D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
   D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& bottomLevelInputs = bottomLevelBuildDesc.Inputs;
   bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
   bottomLevelInputs.Flags = buildFlags;
   bottomLevelInputs.NumDescs = 1;
   bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
   bottomLevelInputs.pGeometryDescs = &geometryDesc;

   D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
   D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& topLevelInputs = topLevelBuildDesc.Inputs;
   topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
   topLevelInputs.Flags = buildFlags;
   topLevelInputs.NumDescs = 1;
   topLevelInputs.pGeometryDescs = nullptr;
   topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

   D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
   dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
   ThrowIfFalse(topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

   D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
   dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
   ThrowIfFalse(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

   ComPtr<ID3D12Resource> scratchResource;
   AllocateUAVBuffer(device.Get(), max(topLevelPrebuildInfo.ScratchDataSizeInBytes, bottomLevelPrebuildInfo.ScratchDataSizeInBytes), &scratchResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"ScratchResource");

   // Allocate resources for acceleration structures.
   // Acceleration structures can only be placed in resources that are created in the default heap (or custom heap equivalent). 
   // Default heap is OK since the application doesn’t need CPU read/write access to them. 
   // The resources that will contain acceleration structures must be created in the state D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, 
   // and must have resource flag D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS. The ALLOW_UNORDERED_ACCESS requirement simply acknowledges both: 
   //  - the system will be doing this type of access in its implementation of acceleration structure builds behind the scenes.
   //  - from the app point of view, synchronization of writes/reads to acceleration structures is accomplished using UAV barriers.
   {
      D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

      AllocateUAVBuffer(device.Get(), bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, &bottomLevelAccelerationStructure, initialResourceState, L"BottomLevelAccelerationStructure");
      AllocateUAVBuffer(device.Get(), topLevelPrebuildInfo.ResultDataMaxSizeInBytes, &topLevelAccelerationStructure, initialResourceState, L"TopLevelAccelerationStructure");
   }

   // Create an instance desc for the bottom-level acceleration structure.
   ComPtr<ID3D12Resource> instanceDescs;
   D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
   instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
   instanceDesc.InstanceMask = 1;
   instanceDesc.AccelerationStructure = bottomLevelAccelerationStructure->GetGPUVirtualAddress();
   AllocateUploadBuffer(device.Get(), &instanceDesc, sizeof(instanceDesc), &instanceDescs, L"InstanceDescs");

   // Bottom Level Acceleration Structure desc
   {
      bottomLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
      bottomLevelBuildDesc.DestAccelerationStructureData = bottomLevelAccelerationStructure->GetGPUVirtualAddress();
   }

   // Top Level Acceleration Structure desc
   {
      topLevelBuildDesc.DestAccelerationStructureData = topLevelAccelerationStructure->GetGPUVirtualAddress();
      topLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
      topLevelBuildDesc.Inputs.InstanceDescs = instanceDescs->GetGPUVirtualAddress();
   }

   auto BuildAccelerationStructure = [&](auto* raytracingCommandList)
   {
      raytracingCommandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
      auto c = CD3DX12_RESOURCE_BARRIER::UAV(bottomLevelAccelerationStructure.Get());
      commandList->ResourceBarrier(1, &c);
      raytracingCommandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
   };

   // Build acceleration structure.
   BuildAccelerationStructure(dxrCommandList.Get());

   // Kick off acceleration structure construction.
   ExecuteCommandList();

   // Wait for GPU to finish as the locally created temporary GPU resources will get released once we go out of scope.
   WaitForGpu();
}

void rnd_Dx12::CreateRaytracingOutputResource()
{
   auto backbufferFormat = swapChainFormat;
   auto& raytracingOutput = textureMgr.rayTracingOutput;

   raytracingOutput.OnInit(backbufferFormat, { window->width, window->height }, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"Raytracing output");
   raytracingOutput.CreateUav();
}

void rnd_Dx12::SerializeAndCreateRaytracingRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, ComPtr<ID3D12RootSignature>* rootSig)
{
   ComPtr<ID3DBlob> blob;
   ComPtr<ID3DBlob> error;

   ThrowIfFailed(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error), error ? static_cast<wchar_t*>(error->GetBufferPointer()) : nullptr);
   ThrowIfFailed(device->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&(*rootSig))));
}

#pragma endregion 

#pragma region Functional
void rnd_Dx12::OnUpdate()
{
   if (imgui)
      imgui->OnUpdate();

   constantBufferMgr.UpdateConstBuffers();

   PopulateGraphicsCommandList();

   MoveToNextFrame();
}

void rnd_Dx12::MoveToNextFrame()
{
   SetBarrier({ {textureMgr.backBuffer[currentFrame], D3D12_RESOURCE_STATE_PRESENT} });

   ExecuteCommandList();

   if (tearingSupported) {
      ThrowIfFailed(swapChain->Present(syncInterval != UINT_MAX ? syncInterval : 0, DXGI_PRESENT_ALLOW_TEARING));
   } else {
      ThrowIfFailed(swapChain->Present(syncInterval != UINT_MAX ? syncInterval : 1, 0));
   }

   const UINT64 currentFenceValue = fenceValues[currentFrame];
   ThrowIfFailed(commandQueue->Signal(fence.Get(), currentFenceValue));

   currentFrame = swapChain->GetCurrentBackBufferIndex();

   if (fence->GetCompletedValue() < fenceValues[currentFrame]) {
      ThrowIfFailed(fence->SetEventOnCompletion(fenceValues[currentFrame], fenceEvent.Get()));
      WaitForSingleObjectEx(fenceEvent.Get(), INFINITE, FALSE);
   }

   fenceValues[currentFrame] = currentFenceValue + 1;
}

void rnd_Dx12::ExecuteCommandList()
{
   ThrowIfFailed(commandList->Close());
   ID3D12CommandList* commandLists[] = { commandList.Get() };
   commandQueue->ExecuteCommandLists(ARRAYSIZE(commandLists), commandLists);
}

void rnd_Dx12::WaitForGpu()
{
   if (commandQueue && fence && fenceEvent.IsValid()) {
      UINT64 fenceValue = fenceValues[currentFrame];
      if (SUCCEEDED(commandQueue->Signal(fence.Get(), fenceValue))) {
         // Wait until the Signal has been processed.
         if (SUCCEEDED(fence->SetEventOnCompletion(fenceValue, fenceEvent.Get()))) {
            WaitForSingleObjectEx(fenceEvent.Get(), INFINITE, FALSE);

            fenceValues[currentFrame]++;
         }
      }
   }
}

void rnd_Dx12::ExecuteCopyCommandList()
{
   ThrowIfFailed(commandListCopy->Close());
   ID3D12CommandList* commandLists[] = { commandListCopy.Get() };
   commandQueueCopy->ExecuteCommandLists(ARRAYSIZE(commandLists), commandLists);
}

void rnd_Dx12::WaitForGpuCopy()
{
   if (commandQueueCopy && fenceCopy && fenceEventCopy.IsValid()) {
      UINT64 fenceValue = fenceValueCopy;
      if (SUCCEEDED(commandQueueCopy->Signal(fenceCopy.Get(), fenceValue))) {
         // Wait until the Signal has been processed.
         if (SUCCEEDED(fenceCopy->SetEventOnCompletion(fenceValue, fenceEventCopy.Get()))) {
            WaitForSingleObjectEx(fenceEventCopy.Get(), INFINITE, FALSE);

            fenceValueCopy++;
         }
      }
   }
}

void rnd_Dx12::SetBarrier(const std::initializer_list<std::pair<rnd_Buffer&, D3D12_RESOURCE_STATES>>& texturesStates)
{
   std::vector<D3D12_RESOURCE_BARRIER> barriers;
   for (auto& texSt : texturesStates) {
      if (texSt.first.state != texSt.second) {
         barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(texSt.first.buffer.Get(), texSt.first.state, texSt.second));
         texSt.first.SetState(texSt.second);
      }
   }
   if (!barriers.empty()) {
      commandList->ResourceBarrier((UINT)barriers.size(),barriers.data());
   }
}

void rnd_Dx12::AddUploadBuffer(ComPtr<ID3D12Resource> uploadBuffer, rnd_Buffer* rndBuffer)
{
   uploadBuffers.push_back({uploadBuffer, rndBuffer}); 
}

void rnd_Dx12::ResolveUploadBuffer()
{
   if (!uploadBuffers.empty())
   {
      ExecuteCopyCommandList(); // TODO: Execute COPY command list!

      WaitForGpuCopy(); // TODO: COPY realization

      for (auto& rndBuffer : uploadBuffers)
      {
         SetBarrier({ {*rndBuffer.second, rndBuffer.second->state} });
         rndBuffer.second->CleanUploadData();
      }
      uploadBuffers.clear();
   }
}

#pragma region Descriptor heap
std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> rnd_Dx12::GetDsvHandle()
{
   --dsvCount;
   assert(dsvCount >= 0);
   auto ret = dsvHandle;
   dsvHandle.first.Offset(1, dsvIncrSize);
   dsvHandle.second.Offset(1, dsvIncrSize);
   return ret;
}

std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> rnd_Dx12::GetRtvHandle()
{
   --rtvCount;
   assert(rtvCount >= 0);
   auto ret = rtvHandle;
   rtvHandle.first.Offset(1, rtvIncrSize);
   rtvHandle.second.Offset(1, rtvIncrSize);
   return ret;
}

std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> rnd_Dx12::GetCbvSrvUavHandle()
{
   --cbvSrvUavCount;
   assert(cbvSrvUavCount >= 0);
   auto ret = cbvSrvUavHandle;
   cbvSrvUavHandle.first.Offset(1, cbvSrvUavIncrSize);
   cbvSrvUavHandle.second.Offset(1, cbvSrvUavIncrSize);
   return ret;
}

std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> rnd_Dx12::GetSamplerHandle()
{
   --svCount;
   assert(svCount >= 0);
   auto ret = samplerHandle;
   samplerHandle.first.Offset(1, svIncrSize);
   samplerHandle.second.Offset(1, svIncrSize);
   return ret;
}
#pragma endregion
#pragma endregion 
