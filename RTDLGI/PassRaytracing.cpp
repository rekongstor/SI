#include "PassRaytracing.h"

#include <Raytracing.hlsl.h>

#include "rnd_Dx12.h"
#include "core_Window.h"
#include "CubeConstBuf.h"
#include "SceneConstBuf.h"
#include "RayTracingHlslCompat.h"


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



void PassRaytracing::OnInit()
{
   cubeCb = dynamic_cast<CubeConstBuf*>(renderer->constantBufferMgr.Get(CUBE_CB));
   sceneCb = dynamic_cast<SceneConstBuf*>(renderer->constantBufferMgr.Get(SCENE_CB));

   m_raytracingLocalRootSignature = renderer->rootSignatureMgr.CreateRootSignature({ Const(SizeOfInUint32(CubeConstantBuffer), 1, 1) }, {}, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);


   m_raytracingGlobalRootSignature = renderer->rootSignatureMgr.CreateRootSignature({
      DescTable({
         DescRange(RngType::UAV, 4, 0)
      }),
      SRV(0),
      CBV(0),
      DescTable({
         DescRange(RngType::SRV, 2, 1)
      })
      });

   CreateRaytracingPipelineStateObject();

   BuildShaderTables();
}

void PassRaytracing::Execute()
{
   auto SetCommonPipelineState = [&](ID3D12GraphicsCommandList* descriptorSetCommandList)
   {
      descriptorSetCommandList->SetDescriptorHeaps(1, renderer->cbvSrvUavHeap.GetAddressOf());
      // Set index and successive vertex buffer descriptor tables
      renderer->CommandList()->SetComputeRootDescriptorTable(GlobalRootSignatureParams::VertexBuffersSlot, renderer->scene.meshes[0].indexBuffer.srvHandle.second);
      renderer->CommandList()->SetComputeRootDescriptorTable(GlobalRootSignatureParams::OutputViewSlot, renderer->textureMgr.giBuffer.uavHandle[0].second);
   };
   renderer->CommandList()->SetComputeRootSignature(m_raytracingGlobalRootSignature.Get());
   SetCommonPipelineState(renderer->CommandList());

   renderer->SetBarrier({ {renderer->textureMgr.giBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS}, {renderer->textureMgr.rayTracingOutput, D3D12_RESOURCE_STATE_UNORDERED_ACCESS} });

   auto frameIndex = renderer->currentFrame;

   auto DispatchRays = [&](auto* commandList, auto* stateObject, auto* dispatchDesc, int resolution)
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
      dispatchDesc->Width = resolution;
      dispatchDesc->Height = resolution;
      dispatchDesc->Depth = resolution;
      commandList->SetPipelineState1(stateObject);
      commandList->DispatchRays(dispatchDesc);
   };


   // Copy the updated scene constant buffer to GPU.
   // This should be already updated from cbMgr->Update()
   auto cbGpuAddress = sceneCb->buffer->GetGPUVirtualAddress() + frameIndex * sizeof(sceneCb->mappedData[0]);
   renderer->CommandList()->SetComputeRootConstantBufferView(GlobalRootSignatureParams::SceneConstantSlot, cbGpuAddress);

   // Bind the heaps, acceleration structure and dispatch rays.
   D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
   renderer->CommandList()->SetComputeRootShaderResourceView(GlobalRootSignatureParams::AccelerationStructureSlot, renderer->scene.topLayerAS.buffer->GetGPUVirtualAddress());
   DispatchRays(renderer->dxrCommandList.Get(), m_dxrStateObject.Get(), &dispatchDesc, GI_RESOLUTION);

   DispatchRays(renderer->dxrCommandList.Get(), m_dxrStateObject.Get(), &dispatchDesc, RAYS_PER_AXIS);
   if (renderer->counter >= 0)
      renderer->counter--;
}

void PassRaytracing::CreateRaytracingPipelineStateObject()
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
   ThrowIfFailed(renderer->dxrDevice->CreateStateObject(raytracingPipeline, IID_PPV_ARGS(&m_dxrStateObject)), L"Couldn't create DirectX Raytracing state object.\n");
}

void PassRaytracing::CreateLocalRootSignatureSubobjects(CD3DX12_STATE_OBJECT_DESC* raytracingPipeline)
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

void PassRaytracing::BuildShaderTables()
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
      ShaderTable rayGenShaderTable(renderer->Device(), numShaderRecords, shaderRecordSize, L"RayGenShaderTable");
      rayGenShaderTable.push_back(ShaderRecord(rayGenShaderIdentifier, shaderIdentifierSize));
      m_rayGenShaderTable = rayGenShaderTable.GetResource();
   }

   // Miss shader table
   {
      UINT numShaderRecords = 1;
      UINT shaderRecordSize = shaderIdentifierSize;
      ShaderTable missShaderTable(renderer->Device(), numShaderRecords, shaderRecordSize, L"MissShaderTable");
      missShaderTable.push_back(ShaderRecord(missShaderIdentifier, shaderIdentifierSize));
      m_missShaderTable = missShaderTable.GetResource();
   }

   // Hit group shader table
   {
      struct RootArguments {
         CubeConstantBuffer cb;
      } rootArguments;
      rootArguments.cb.albedo = cubeCb->albedo;

      UINT numShaderRecords = 1;
      UINT shaderRecordSize = shaderIdentifierSize + sizeof(rootArguments);
      ShaderTable hitGroupShaderTable(renderer->Device(), numShaderRecords, shaderRecordSize, L"HitGroupShaderTable");
      hitGroupShaderTable.push_back(ShaderRecord(hitGroupShaderIdentifier, shaderIdentifierSize, &rootArguments, sizeof(rootArguments)));
      m_hitGroupShaderTable = hitGroupShaderTable.GetResource();
   }
}
