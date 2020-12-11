#include "rnd_Dx12.h"
#include "core_Window.h"
#include "core_Imgui.h"
#include "RayTracingHlslCompat.h"
#include <Raytracing.hlsl.h>

wchar_t name[256]{ 0 };

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

inline void PrintStateObjectDesc(const D3D12_STATE_OBJECT_DESC* desc)
{
   // Pretty-print a state object tree.
   std::wstringstream wstr;
   wstr << L"\n";
   wstr << L"--------------------------------------------------------------------\n";
   wstr << L"| D3D12 State Object 0x" << static_cast<const void*>(desc) << L": ";
   if (desc->Type == D3D12_STATE_OBJECT_TYPE_COLLECTION) wstr << L"Collection\n";
   if (desc->Type == D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE) wstr << L"Raytracing Pipeline\n";

   auto ExportTree = [](UINT depth, UINT numExports, const D3D12_EXPORT_DESC* exports)
   {
      std::wostringstream woss;
      for (UINT i = 0; i < numExports; i++) {
         woss << L"|";
         if (depth > 0) {
            for (UINT j = 0; j < 2 * depth - 1; j++) woss << L" ";
         }
         woss << L" [" << i << L"]: ";
         if (exports[i].ExportToRename) woss << exports[i].ExportToRename << L" --> ";
         woss << exports[i].Name << L"\n";
      }
      return woss.str();
   };

   for (UINT i = 0; i < desc->NumSubobjects; i++) {
      wstr << L"| [" << i << L"]: ";
      switch (desc->pSubobjects[i].Type) {
      case D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE:
         wstr << L"Global Root Signature 0x" << desc->pSubobjects[i].pDesc << L"\n";
         break;
      case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE:
         wstr << L"Local Root Signature 0x" << desc->pSubobjects[i].pDesc << L"\n";
         break;
      case D3D12_STATE_SUBOBJECT_TYPE_NODE_MASK:
         wstr << L"Node Mask: 0x" << std::hex << std::setfill(L'0') << std::setw(8) << *static_cast<const UINT*>(desc->pSubobjects[i].pDesc) << std::setw(0) << std::dec << L"\n";
         break;
      case D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY:
      {
         wstr << L"DXIL Library 0x";
         auto lib = static_cast<const D3D12_DXIL_LIBRARY_DESC*>(desc->pSubobjects[i].pDesc);
         wstr << lib->DXILLibrary.pShaderBytecode << L", " << lib->DXILLibrary.BytecodeLength << L" bytes\n";
         wstr << ExportTree(1, lib->NumExports, lib->pExports);
         break;
      }
      case D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION:
      {
         wstr << L"Existing Library 0x";
         auto collection = static_cast<const D3D12_EXISTING_COLLECTION_DESC*>(desc->pSubobjects[i].pDesc);
         wstr << collection->pExistingCollection << L"\n";
         wstr << ExportTree(1, collection->NumExports, collection->pExports);
         break;
      }
      case D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION:
      {
         wstr << L"Sub-object to Exports Association (Sub-object [";
         auto association = static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(desc->pSubobjects[i].pDesc);
         UINT index = static_cast<UINT>(association->pSubobjectToAssociate - desc->pSubobjects);
         wstr << index << L"])\n";
         for (UINT j = 0; j < association->NumExports; j++) {
            wstr << L"|  [" << j << L"]: " << association->pExports[j] << L"\n";
         }
         break;
      }
      case D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION:
      {
         wstr << L"DXIL Sub-objects to Exports Association (";
         auto association = static_cast<const D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(desc->pSubobjects[i].pDesc);
         wstr << association->SubobjectToAssociate << L")\n";
         for (UINT j = 0; j < association->NumExports; j++) {
            wstr << L"|  [" << j << L"]: " << association->pExports[j] << L"\n";
         }
         break;
      }
      case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG:
      {
         wstr << L"Raytracing Shader Config\n";
         auto config = static_cast<const D3D12_RAYTRACING_SHADER_CONFIG*>(desc->pSubobjects[i].pDesc);
         wstr << L"|  [0]: Max Payload Size: " << config->MaxPayloadSizeInBytes << L" bytes\n";
         wstr << L"|  [1]: Max Attribute Size: " << config->MaxAttributeSizeInBytes << L" bytes\n";
         break;
      }
      case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG:
      {
         wstr << L"Raytracing Pipeline Config\n";
         auto config = static_cast<const D3D12_RAYTRACING_PIPELINE_CONFIG*>(desc->pSubobjects[i].pDesc);
         wstr << L"|  [0]: Max Recursion Depth: " << config->MaxTraceRecursionDepth << L"\n";
         break;
      }
      case D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP:
      {
         wstr << L"Hit Group (";
         auto hitGroup = static_cast<const D3D12_HIT_GROUP_DESC*>(desc->pSubobjects[i].pDesc);
         wstr << (hitGroup->HitGroupExport ? hitGroup->HitGroupExport : L"[none]") << L")\n";
         wstr << L"|  [0]: Any Hit Import: " << (hitGroup->AnyHitShaderImport ? hitGroup->AnyHitShaderImport : L"[none]") << L"\n";
         wstr << L"|  [1]: Closest Hit Import: " << (hitGroup->ClosestHitShaderImport ? hitGroup->ClosestHitShaderImport : L"[none]") << L"\n";
         wstr << L"|  [2]: Intersection Import: " << (hitGroup->IntersectionShaderImport ? hitGroup->IntersectionShaderImport : L"[none]") << L"\n";
         break;
      }
      }
      wstr << L"|--------------------------------------------------------------------\n";
   }
   wstr << L"\n";
   OutputDebugStringW(wstr.str().c_str());
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
      swprintf(name, _countof(name), L"COMMAND ALLOCATOR %d: Direct", i);
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
   commandListCopy->Close();
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
      swprintf(name, _countof(name), L"BackBuffer_%d", i);
      ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));
      textureMgr.backBuffer[i].OnInit(backBuffer, swapChainFormat, D3D12_RESOURCE_STATE_PRESENT, name);
      textureMgr.backBuffer[i].CreateRtv(device.Get(), GetRtvHandle());
   }
#pragma endregion

   InitRaytracing();

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

   DoRaytracing();
   CopyRaytracingOutputToBackbuffer();

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
   // Initialize Raytracing pipeline.

   {
      m_cubeCB.albedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
   }

   // Initialize the view and projection inverse matrices.
   m_eye = { 0.0f, 2.0f, -5.0f, 1.0f };
   m_at = { 0.0f, 0.0f, 0.0f, 1.0f };
   XMVECTOR right = { 1.0f, 0.0f, 0.0f, 0.0f };

   XMVECTOR direction = XMVector4Normalize(m_at - m_eye);
   m_up = XMVector3Normalize(XMVector3Cross(direction, right));

   // Rotate camera around Y axis.
   XMMATRIX rotate = XMMatrixRotationY(XMConvertToRadians(45.0f));
   m_eye = XMVector3Transform(m_eye, rotate);
   m_up = XMVector3Transform(m_up, rotate);

   // Setup lights.
   {
      // Initialize the lighting parameters.
      XMFLOAT4 lightPosition;
      XMFLOAT4 lightAmbientColor;
      XMFLOAT4 lightDiffuseColor;

      lightPosition = XMFLOAT4(0.0f, 1.8f, -3.0f, 0.0f);
      m_sceneCB[renderer->currentFrame].lightPosition = XMLoadFloat4(&lightPosition);

      lightAmbientColor = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
      m_sceneCB[renderer->currentFrame].lightAmbientColor = XMLoadFloat4(&lightAmbientColor);

      lightDiffuseColor = XMFLOAT4(0.5f, 0.0f, 0.0f, 1.0f);
      m_sceneCB[renderer->currentFrame].lightDiffuseColor = XMLoadFloat4(&lightDiffuseColor);
   }

   // Apply the initial values to all frames' buffer instances.
   for (auto& sceneCB : m_sceneCB) {
      sceneCB = m_sceneCB[renderer->currentFrame];
   }

   UpdateCameraMatrices();

   // Create raytracing interfaces: raytracing device and commandlist.
   CreateRaytracingInterfaces();

   // Create root signatures for the shaders.
   CreateRootSignatures();

   // Create a raytracing pipeline state object which defines the binding of shaders, state and resources to be used during raytracing.
   CreateRaytracingPipelineStateObject();

   // Build geometry to be used in the sample.
   BuildGeometry();

   // Build raytracing acceleration structures from the generated geometry.
   BuildAccelerationStructures();

   // Create constant buffers for the geometry and the scene.
   CreateConstantBuffers();

   // Build shader tables, which define shaders and their local root arguments.
   BuildShaderTables();

   // Create an output 2D texture to store the raytracing result to.
   CreateRaytracingOutputResource();
}

void rnd_Dx12::UpdateCameraMatrices()
{
   // Update camera matrices passed into the shader.
   auto frameIndex = currentFrame;

   m_sceneCB[frameIndex].cameraPosition = m_eye;
   float fovAngleY = 45.0f;
   XMMATRIX view = XMMatrixLookAtLH(m_eye, m_at, m_up);
   XMMATRIX proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(fovAngleY), (float)window->width / (float)window->height, 1.0f, 125.0f);
   XMMATRIX viewProj = view * proj;

   m_sceneCB[frameIndex].projectionToWorld = XMMatrixInverse(nullptr, viewProj);
}

void rnd_Dx12::DoRaytracing()
{
   // Rotate the camera around Y axis.
   {
      float secondsToRotateAround = 24.0f;
      float angleToRotateBy = 360.0f * (0.f / secondsToRotateAround);
      XMMATRIX rotate = XMMatrixRotationY(XMConvertToRadians(angleToRotateBy));
      m_eye = XMVector3Transform(m_eye, rotate);
      m_up = XMVector3Transform(m_up, rotate);
      m_at = XMVector3Transform(m_at, rotate);
      UpdateCameraMatrices();
   }

   // Rotate the second light around Y axis.
   {
      float secondsToRotateAround = 8.0f;
      float angleToRotateBy = -360.0f * (0.f / secondsToRotateAround);
      XMMATRIX rotate = XMMatrixRotationY(XMConvertToRadians(angleToRotateBy));
      const XMVECTOR& prevLightPosition = m_sceneCB[PreviousFrame()].lightPosition;
      m_sceneCB[currentFrame].lightPosition = XMVector3Transform(prevLightPosition, rotate);
   }

   auto frameIndex = currentFrame;

   auto DispatchRays = [&](auto* commandList, auto* stateObject, auto* dispatchDesc)
   {
      // Since each shader table has only one shader record, the stride is same as the size.
      dispatchDesc->HitGroupTable.StartAddress = m_hitGroupShaderTable->GetGPUVirtualAddress();
      dispatchDesc->HitGroupTable.SizeInBytes = m_hitGroupShaderTable->GetDesc().Width;
      dispatchDesc->HitGroupTable.StrideInBytes = dispatchDesc->HitGroupTable.SizeInBytes;
      dispatchDesc->MissShaderTable.StartAddress = m_missShaderTable->GetGPUVirtualAddress();
      dispatchDesc->MissShaderTable.SizeInBytes = m_missShaderTable->GetDesc().Width;
      dispatchDesc->MissShaderTable.StrideInBytes = dispatchDesc->MissShaderTable.SizeInBytes;
      dispatchDesc->RayGenerationShaderRecord.StartAddress = m_rayGenShaderTable->GetGPUVirtualAddress();
      dispatchDesc->RayGenerationShaderRecord.SizeInBytes = m_rayGenShaderTable->GetDesc().Width;
      dispatchDesc->Width = window->width;
      dispatchDesc->Height = window->height;
      dispatchDesc->Depth = 1;
      commandList->SetPipelineState1(stateObject);
      commandList->DispatchRays(dispatchDesc);
   };

   auto SetCommonPipelineState = [&](ID3D12GraphicsCommandList* descriptorSetCommandList)
   {
      descriptorSetCommandList->SetDescriptorHeaps(1, cbvSrvUavHeap.GetAddressOf());
      // Set index and successive vertex buffer descriptor tables
      commandList->SetComputeRootDescriptorTable(GlobalRootSignatureParams::VertexBuffersSlot, m_indexBuffer.srvDescHandle.second);
      commandList->SetComputeRootDescriptorTable(GlobalRootSignatureParams::OutputViewSlot, textureMgr.rayTracingOutput.srvHandle.second);
   };

   commandList->SetComputeRootSignature(m_raytracingGlobalRootSignature.Get());

   // Copy the updated scene constant buffer to GPU.
   memcpy(&m_mappedConstantData[frameIndex].constants, &m_sceneCB[frameIndex], sizeof(m_sceneCB[frameIndex]));
   auto cbGpuAddress = m_perFrameConstants->GetGPUVirtualAddress() + frameIndex * sizeof(m_mappedConstantData[0]);
   commandList->SetComputeRootConstantBufferView(GlobalRootSignatureParams::SceneConstantSlot, cbGpuAddress);

   // Bind the heaps, acceleration structure and dispatch rays.
   D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
   SetCommonPipelineState(commandList.Get());
   commandList->SetComputeRootShaderResourceView(GlobalRootSignatureParams::AccelerationStructureSlot, m_topLevelAccelerationStructure->GetGPUVirtualAddress());
   DispatchRays(m_dxrCommandList.Get(), m_dxrStateObject.Get(), &dispatchDesc);
}

void rnd_Dx12::CopyRaytracingOutputToBackbuffer()
{
   auto& renderTarget = textureMgr.backBuffer[currentFrame];
   auto& rayTracingOutput = textureMgr.rayTracingOutput;

   SetBarrier({ {renderTarget, D3D12_RESOURCE_STATE_COPY_DEST}, {rayTracingOutput, D3D12_RESOURCE_STATE_COPY_SOURCE} });

   commandList->CopyResource(renderTarget.buffer.Get(), rayTracingOutput.buffer.Get());

   SetBarrier({ {renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET}, {rayTracingOutput, D3D12_RESOURCE_STATE_UNORDERED_ACCESS} }); // TODO: Do I need this?
}

void rnd_Dx12::CreateRaytracingInterfaces()
{
   ThrowIfFailed(device->QueryInterface(IID_PPV_ARGS(&m_dxrDevice)), L"Couldn't get DirectX Raytracing interface for the device.\n");
   ThrowIfFailed(commandList->QueryInterface(IID_PPV_ARGS(&m_dxrCommandList)), L"Couldn't get DirectX Raytracing interface for the command list.\n");
}

void rnd_Dx12::CreateRootSignatures()
{
   // Global Root Signature
   // This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
   {
      CD3DX12_DESCRIPTOR_RANGE ranges[2]; // Performance TIP: Order from most frequent to least frequent.
      ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);  // 1 output texture
      ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1);  // 2 static index and vertex buffers.

      CD3DX12_ROOT_PARAMETER rootParameters[GlobalRootSignatureParams::Count];
      rootParameters[GlobalRootSignatureParams::OutputViewSlot].InitAsDescriptorTable(1, &ranges[0]);
      rootParameters[GlobalRootSignatureParams::AccelerationStructureSlot].InitAsShaderResourceView(0);
      rootParameters[GlobalRootSignatureParams::SceneConstantSlot].InitAsConstantBufferView(0);
      rootParameters[GlobalRootSignatureParams::VertexBuffersSlot].InitAsDescriptorTable(1, &ranges[1]);
      CD3DX12_ROOT_SIGNATURE_DESC globalRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
      SerializeAndCreateRaytracingRootSignature(globalRootSignatureDesc, &m_raytracingGlobalRootSignature);
   }

   // Local Root Signature
   // This is a root signature that enables a shader to have unique arguments that come from shader tables.
   {
      CD3DX12_ROOT_PARAMETER rootParameters[LocalRootSignatureParams::Count];
      rootParameters[LocalRootSignatureParams::CubeConstantSlot].InitAsConstants(SizeOfInUint32(m_cubeCB), 1);
      CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
      localRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
      SerializeAndCreateRaytracingRootSignature(localRootSignatureDesc, &m_raytracingLocalRootSignature);
   }
}

void rnd_Dx12::CreateRaytracingPipelineStateObject()
{
   // Create 7 sub-objects that combine into a RTPSO:
   // Sub-objects need to be associated with DXIL exports (i.e. shaders) either by way of default or explicit associations.
   // Default association applies to every exported shader entrypoint that doesn't have any of the same type of sub-object associated with it.
   // This simple sample utilizes default shader association except for local root signature sub-object
   // which has an explicit association specified purely for demonstration purposes.
   // 1 - DXIL library
   // 1 - Triangle hit group
   // 1 - Shader config
   // 2 - Local root signature and association
   // 1 - Global root signature
   // 1 - Pipeline config
   CD3DX12_STATE_OBJECT_DESC raytracingPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };


   // DXIL library
   // This contains the shaders and their entry points for the state object.
   // Since shaders are not considered a sub-object, they need to be passed in via DXIL library sub-objects.
   auto lib = raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
   D3D12_SHADER_BYTECODE libdxil = CD3DX12_SHADER_BYTECODE((void*)g_pRaytracing, ARRAYSIZE(g_pRaytracing));
   lib->SetDXILLibrary(&libdxil);
   // Define which shader exports to surface from the library.
   // If no shader exports are defined for a DXIL library sub-object, all shaders will be surfaced.
   // In this sample, this could be omitted for convenience since the sample uses all shaders in the library.
   {
      lib->DefineExport(rayGenShaderName);
      lib->DefineExport(closestHitShaderName);
      lib->DefineExport(missShaderName);
   }

   // Triangle hit group
   // A hit group specifies closest hit, any hit and intersection shaders to be executed when a ray intersects the geometry's triangle/AABB.
   // In this sample, we only use triangle geometry with a closest hit shader, so others are not set.
   auto hitGroup = raytracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
   hitGroup->SetClosestHitShaderImport(closestHitShaderName);
   hitGroup->SetHitGroupExport(hitGroupName);
   hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

   // Shader config
   // Defines the maximum sizes in bytes for the ray payload and attribute structure.
   auto shaderConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
   UINT payloadSize = sizeof(XMFLOAT4);    // float4 pixelColor
   UINT attributeSize = sizeof(XMFLOAT2);  // float2 barycentric
   shaderConfig->Config(payloadSize, attributeSize);

   // Local root signature and shader association
   // This is a root signature that enables a shader to have unique arguments that come from shader tables.
   CreateLocalRootSignatureSubobjects(&raytracingPipeline);

   // Global root signature
   // This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
   auto globalRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
   globalRootSignature->SetRootSignature(m_raytracingGlobalRootSignature.Get());

   // Pipeline config
   // Defines the maximum TraceRay() recursion depth.
   auto pipelineConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
   // PERFOMANCE TIP: Set max recursion depth as low as needed 
   // as drivers may apply optimization strategies for low recursion depths.
   UINT maxRecursionDepth = 1; // ~ primary rays only. 
   pipelineConfig->Config(maxRecursionDepth);

#if _DEBUG
   PrintStateObjectDesc(raytracingPipeline);
#endif

   // Create the state object.
   ThrowIfFailed(m_dxrDevice->CreateStateObject(raytracingPipeline, IID_PPV_ARGS(&m_dxrStateObject)), L"Couldn't create DirectX Raytracing state object.\n");
}

void rnd_Dx12::BuildGeometry()
{
   // Cube indices.
   Index indices[] =
   {
       3,1,0,
       2,1,3,

       6,4,5,
       7,4,6,

       11,9,8,
       10,9,11,

       14,12,13,
       15,12,14,

       19,17,16,
       18,17,19,

       22,20,21,
       23,20,22
   };

   // Cube vertices positions and corresponding triangle normals.
   Vertex vertices[] =
   {
       { XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
       { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
       { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
       { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },

       { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
       { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
       { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
       { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },

       { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
       { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
       { XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
       { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },

       { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
       { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
       { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
       { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },

       { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
       { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
       { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
       { XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },

       { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
       { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
       { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
       { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
   };

   AllocateUploadBuffer(device.Get(), indices, sizeof(indices), &m_indexBuffer.buffer);
   AllocateUploadBuffer(device.Get(), vertices, sizeof(vertices), &m_vertexBuffer.buffer);

   // Vertex buffer is passed to the shader along with index buffer as a descriptor table.
   // Vertex buffer descriptor must follow index buffer descriptor in the descriptor heap.
   CreateBufferSRV(&m_indexBuffer, sizeof(indices) / 4, 0);
   CreateBufferSRV(&m_vertexBuffer, ARRAYSIZE(vertices), sizeof(vertices[0]));
}

void rnd_Dx12::BuildAccelerationStructures()
{
   auto commandAllocator = commandAllocators[currentFrame].Get();

   // Reset the command list for the acceleration structure construction.
   commandList->Reset(commandAllocator, nullptr);

   D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
   geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
   geometryDesc.Triangles.IndexBuffer = m_indexBuffer.buffer->GetGPUVirtualAddress();
   geometryDesc.Triangles.IndexCount = static_cast<UINT>(m_indexBuffer.buffer->GetDesc().Width) / sizeof(Index);
   geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
   geometryDesc.Triangles.Transform3x4 = 0;
   geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
   geometryDesc.Triangles.VertexCount = static_cast<UINT>(m_vertexBuffer.buffer->GetDesc().Width) / sizeof(Vertex);
   geometryDesc.Triangles.VertexBuffer.StartAddress = m_vertexBuffer.buffer->GetGPUVirtualAddress();
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
   m_dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
   ThrowIfFalse(topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

   D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
   m_dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
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

      AllocateUAVBuffer(device.Get(), bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_bottomLevelAccelerationStructure, initialResourceState, L"BottomLevelAccelerationStructure");
      AllocateUAVBuffer(device.Get(), topLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_topLevelAccelerationStructure, initialResourceState, L"TopLevelAccelerationStructure");
   }

   // Create an instance desc for the bottom-level acceleration structure.
   ComPtr<ID3D12Resource> instanceDescs;
   D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
   instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
   instanceDesc.InstanceMask = 1;
   instanceDesc.AccelerationStructure = m_bottomLevelAccelerationStructure->GetGPUVirtualAddress();
   AllocateUploadBuffer(device.Get(), &instanceDesc, sizeof(instanceDesc), &instanceDescs, L"InstanceDescs");

   // Bottom Level Acceleration Structure desc
   {
      bottomLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
      bottomLevelBuildDesc.DestAccelerationStructureData = m_bottomLevelAccelerationStructure->GetGPUVirtualAddress();
   }

   // Top Level Acceleration Structure desc
   {
      topLevelBuildDesc.DestAccelerationStructureData = m_topLevelAccelerationStructure->GetGPUVirtualAddress();
      topLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
      topLevelBuildDesc.Inputs.InstanceDescs = instanceDescs->GetGPUVirtualAddress();
   }

   auto BuildAccelerationStructure = [&](auto* raytracingCommandList)
   {
      raytracingCommandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
      auto c = CD3DX12_RESOURCE_BARRIER::UAV(m_bottomLevelAccelerationStructure.Get());
      commandList->ResourceBarrier(1, &c);
      raytracingCommandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
   };

   // Build acceleration structure.
   BuildAccelerationStructure(m_dxrCommandList.Get());

   // Kick off acceleration structure construction.
   ExecuteCommandList();

   // Wait for GPU to finish as the locally created temporary GPU resources will get released once we go out of scope.
   WaitForGpu();
}

void rnd_Dx12::CreateConstantBuffers()
{
   // Create the constant buffer memory and map the CPU and GPU addresses
   const D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

   // Allocate one constant buffer per frame, since it gets updated every frame.
   size_t cbSize = FRAME_COUNT * sizeof(AlignedSceneConstantBuffer);
   const int i = sizeof(AlignedSceneConstantBuffer);
   const D3D12_RESOURCE_DESC constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);

   ThrowIfFailed(device->CreateCommittedResource(
      &uploadHeapProperties,
      D3D12_HEAP_FLAG_NONE,
      &constantBufferDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&m_perFrameConstants)));

   // Map the constant buffer and cache its heap pointers.
   // We don't unmap this until the app closes. Keeping buffer mapped for the lifetime of the resource is okay.
   CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
   ThrowIfFailed(m_perFrameConstants->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedConstantData)));
}

void rnd_Dx12::BuildShaderTables()
{
   void* rayGenShaderIdentifier;
   void* missShaderIdentifier;
   void* hitGroupShaderIdentifier;

   auto GetShaderIdentifiers = [&](auto* stateObjectProperties)
   {
      rayGenShaderIdentifier = stateObjectProperties->GetShaderIdentifier(rayGenShaderName);
      missShaderIdentifier = stateObjectProperties->GetShaderIdentifier(missShaderName);
      hitGroupShaderIdentifier = stateObjectProperties->GetShaderIdentifier(hitGroupName);
   };

   // Get shader identifiers.
   UINT shaderIdentifierSize;
   {
      ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
      ThrowIfFailed(m_dxrStateObject.As(&stateObjectProperties));
      GetShaderIdentifiers(stateObjectProperties.Get());
      shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
   }

   // Ray gen shader table
   {
      UINT numShaderRecords = 1;
      UINT shaderRecordSize = shaderIdentifierSize;
      ShaderTable rayGenShaderTable(device.Get(), numShaderRecords, shaderRecordSize, L"RayGenShaderTable");
      rayGenShaderTable.push_back(ShaderRecord(rayGenShaderIdentifier, shaderIdentifierSize));
      m_rayGenShaderTable = rayGenShaderTable.GetResource();
   }

   // Miss shader table
   {
      UINT numShaderRecords = 1;
      UINT shaderRecordSize = shaderIdentifierSize;
      ShaderTable missShaderTable(device.Get(), numShaderRecords, shaderRecordSize, L"MissShaderTable");
      missShaderTable.push_back(ShaderRecord(missShaderIdentifier, shaderIdentifierSize));
      m_missShaderTable = missShaderTable.GetResource();
   }

   // Hit group shader table
   {
      struct RootArguments {
         CubeConstantBuffer cb;
      } rootArguments;
      rootArguments.cb = m_cubeCB;

      UINT numShaderRecords = 1;
      UINT shaderRecordSize = shaderIdentifierSize + sizeof(rootArguments);
      ShaderTable hitGroupShaderTable(device.Get(), numShaderRecords, shaderRecordSize, L"HitGroupShaderTable");
      hitGroupShaderTable.push_back(ShaderRecord(hitGroupShaderIdentifier, shaderIdentifierSize, &rootArguments, sizeof(rootArguments)));
      m_hitGroupShaderTable = hitGroupShaderTable.GetResource();
   }
}

void rnd_Dx12::CreateRaytracingOutputResource()
{
   auto backbufferFormat = swapChainFormat;
   auto& raytracingOutput = textureMgr.rayTracingOutput;

   // Create the output resource. The dimensions and format should match the swap-chain.
   auto uavDesc = CD3DX12_RESOURCE_DESC::Tex2D(backbufferFormat, window->width, window->height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

   auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
   ThrowIfFailed(device->CreateCommittedResource(
      &defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &uavDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&raytracingOutput.buffer)));
   raytracingOutput.buffer.Get()->SetName(L"Raytracing output");
   raytracingOutput.state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

   auto descHandle = GetCbvSrvUavHandle();
   raytracingOutput.srvHandle.first = descHandle.first;
   raytracingOutput.srvHandle.second = descHandle.second;
   D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
   UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
   device->CreateUnorderedAccessView(raytracingOutput.buffer.Get(), nullptr, &UAVDesc, descHandle.first);
}

void rnd_Dx12::SerializeAndCreateRaytracingRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, ComPtr<ID3D12RootSignature>* rootSig)
{
   ComPtr<ID3DBlob> blob;
   ComPtr<ID3DBlob> error;

   ThrowIfFailed(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error), error ? static_cast<wchar_t*>(error->GetBufferPointer()) : nullptr);
   ThrowIfFailed(device->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&(*rootSig))));
}

void rnd_Dx12::CreateLocalRootSignatureSubobjects(CD3DX12_STATE_OBJECT_DESC* raytracingPipeline)
{
   // Ray gen and miss shaders in this sample are not using a local root signature and thus one is not associated with them.

   // Local root signature to be used in a hit group.
   auto localRootSignature = raytracingPipeline->CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
   localRootSignature->SetRootSignature(m_raytracingLocalRootSignature.Get());
   // Define explicit shader association for the local root signature. 
   {
      auto rootSignatureAssociation = raytracingPipeline->CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
      rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignature);
      rootSignatureAssociation->AddExport(hitGroupName);
   }
}

void rnd_Dx12::CreateBufferSRV(SrvBuffer* srvBuffer, int numElements, int strideInBytes)
{
   // SRV
   D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
   srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
   srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
   srvDesc.Buffer.NumElements = numElements;
   if (strideInBytes == 0) {
      srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
      srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
      srvDesc.Buffer.StructureByteStride = 0;
   } else {
      srvDesc.Format = DXGI_FORMAT_UNKNOWN;
      srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
      srvDesc.Buffer.StructureByteStride = strideInBytes;
   }
   auto descHandle = renderer->GetCbvSrvUavHandle();
   srvBuffer->srvDescHandle.first = descHandle.first;
   srvBuffer->srvDescHandle.second = descHandle.second;
   device->CreateShaderResourceView(srvBuffer->buffer.Get(), &srvDesc, descHandle.first);
}
#pragma endregion 

#pragma region Functional
void rnd_Dx12::OnUpdate()
{
   if (imgui)
      imgui->OnUpdate();

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

void rnd_Dx12::SetBarrier(const std::vector<std::pair<rnd_Texture&, D3D12_RESOURCE_STATES>>& texturesStates)
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
