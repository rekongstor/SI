#include "rtAccelerationStructures.h"

#include <d3dcompiler.h>
#include <fstream>



#include "alignment.h"
#include "siCommandAllocator.h"
#include "siCommandQueue.h"
#include "siMesh.h"

void rtAccelerationStructures::AddMeshToGeometryDesc(siMesh* pMesh)
{
   geometryDescsAssociationMap[pMesh] = geometryDescs.size();
   geometryDescs.push_back({});
   auto& geomDesc = geometryDescs.back();

   geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
   geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAGS::D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

   geomDesc.Triangles.IndexBuffer = pMesh->getIndexBufferView().BufferLocation;
   geomDesc.Triangles.IndexCount = pMesh->getIndexCount();
   geomDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
   geomDesc.Triangles.Transform3x4 = 0;
   geomDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
   geomDesc.Triangles.VertexCount = pMesh->getVertexCount();
   geomDesc.Triangles.VertexBuffer.StartAddress = pMesh->getVertexBufferView().BufferLocation;
   geomDesc.Triangles.VertexBuffer.StrideInBytes = pMesh->getVertexBufferView().StrideInBytes;
}

void rtAccelerationStructures::OnInit(ID3D12Device5* device, ID3D12GraphicsCommandList5* commandList)
{
   dxrCommandList = reinterpret_cast<ID3D12GraphicsCommandList5*>(commandList);
   commandList->QueryInterface(IID_PPV_ARGS(&dxrCommandList));

   //commandList->Reset(commandAllocator.GetAllocator(0), nullptr);

   tlasInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
   tlasInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
   tlasInputs.NumDescs = 1;
   tlasInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

   blasInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT::D3D12_ELEMENTS_LAYOUT_ARRAY;
   blasInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
   blasInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
   blasInputs.pGeometryDescs = geometryDescs.data();
   blasInputs.NumDescs = geometryDescs.size();

   D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfoTlas {}, prebuildInfoBlas {};
   device->GetRaytracingAccelerationStructurePrebuildInfo(&tlasInputs, &prebuildInfoTlas);
   device->GetRaytracingAccelerationStructurePrebuildInfo(&blasInputs, &prebuildInfoBlas);


   HRESULT hr = device->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
      D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Buffer(AlignSize(max(prebuildInfoTlas.ScratchDataSizeInBytes, prebuildInfoBlas.ScratchDataSizeInBytes), D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT), D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
      D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
      nullptr,
      IID_PPV_ARGS(&scratchData)
   );
   scratchData->SetName(L"scratchData BLAS");
   assert(hr == S_OK);

   hr = device->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
      D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Buffer(AlignSize(prebuildInfoBlas.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT), D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
      D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
      nullptr,
      IID_PPV_ARGS(&destDataBlas)
   );
   destDataBlas->SetName(L"destData BLAS");
   assert(hr == S_OK);

   hr = device->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
      D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Buffer(AlignSize(prebuildInfoTlas.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT), D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
      D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
      nullptr,
      IID_PPV_ARGS(&destDataTlas)
   );
   destDataTlas->SetName(L"destData TLAS");
   assert(hr == S_OK);

   D3D12_RAYTRACING_INSTANCE_DESC instanceDesc{};
   instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
   instanceDesc.InstanceMask = 1;
   instanceDesc.AccelerationStructure = destDataBlas.Get()->GetGPUVirtualAddress();

   hr = device->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
      D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Buffer(sizeof(instanceDesc)),
      D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&instanceDescs)
   );
   void* pMappedData;
   instanceDescs.Get()->Map(0, nullptr, &pMappedData);
   memcpy(pMappedData, &instanceDesc, sizeof(instanceDesc));
   instanceDescs.Get()->Unmap(0, nullptr);

   blasDesc.Inputs = blasInputs;
   blasDesc.ScratchAccelerationStructureData = scratchData->GetGPUVirtualAddress();
   blasDesc.DestAccelerationStructureData = destDataBlas->GetGPUVirtualAddress();

   tlasInputs.InstanceDescs = instanceDescs.Get()->GetGPUVirtualAddress();
   tlasDesc.Inputs = tlasInputs;
   tlasDesc.DestAccelerationStructureData = destDataTlas.Get()->GetGPUVirtualAddress();
   tlasDesc.ScratchAccelerationStructureData = scratchData.Get()->GetGPUVirtualAddress();


   dxrCommandList->BuildRaytracingAccelerationStructure(&blasDesc, 0, nullptr);
   commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(destDataBlas.Get()));
   dxrCommandList->BuildRaytracingAccelerationStructure(&tlasDesc, 0, nullptr);

   // Create 7 subobjects that combine into a RTPSO:
   // Subobjects need to be associated with DXIL exports (i.e. shaders) either by way of default or explicit associations.
   // Default association applies to every exported shader entrypoint that doesn't have any of the same type of subobject associated with it.
   // This simple sample utilizes default shader association except for local root signature subobject
   // which has an explicit association specified purely for demonstration purposes.
   // 1 - DXIL library
   // 1 - Triangle hit group
   // 1 - Shader config
   // 2 - Local root signature and association
   // 1 - Global root signature
   // 1 - Pipeline config

   // set shader
   CD3DX12_STATE_OBJECT_DESC rtPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };
   auto lib = rtPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();


   std::string hlslCode;
   {
      std::ifstream is("rt.cso", std::ios::binary);
      int length;
      is.seekg(0, std::ios::end);
      length = is.tellg();
      hlslCode.resize(length);
      is.seekg(0, std::ios::beg);
      is.read(hlslCode.data(), length);
   }

   const wchar_t* c_hitGroupName = L"MyHitGroup";
   const wchar_t* c_raygenShaderName = L"MyRaygenShader";
   const wchar_t* c_closestHitShaderName = L"MyClosestHitShader";
   const wchar_t* c_missShaderName = L"MyMissShader";

   D3D12_SHADER_BYTECODE rtxShaderByteCode = { hlslCode.data(), hlslCode.length() };
   lib->SetDXILLibrary(&rtxShaderByteCode);
   lib->DefineExport(c_raygenShaderName);
   lib->DefineExport(c_closestHitShaderName);
   lib->DefineExport(c_missShaderName);

   auto hitGroup = rtPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
   hitGroup->SetClosestHitShaderImport(c_closestHitShaderName);
   hitGroup->SetHitGroupExport(c_hitGroupName);
   hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

   auto shaderConfig = rtPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
   UINT payloadSize = 4 * sizeof(float);   // float4 color
   UINT attributeSize = 2 * sizeof(float); // float2 barycentrics
   shaderConfig->Config(payloadSize, attributeSize);

   // create local root signature
   rtLocalRootSignature.onInit(device, siRootSignature::createRtLocalRootSignature());
   
   auto localRootSignature = rtPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
   localRootSignature->SetRootSignature(rtLocalRootSignature.get().Get());

   auto rootSignatureAssociation = rtPipeline.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
   rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignature);
   rootSignatureAssociation->AddExport(c_raygenShaderName);

   // create global root signature
   rtGlobalRootSignature.onInit(device, siRootSignature::createRtGlobalRootSignature());

   auto globalRootSignature = rtPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
   globalRootSignature->SetRootSignature(rtGlobalRootSignature.get().Get());

   auto pipelineConfig = rtPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
   UINT maxRecursionDepth = 1;
   pipelineConfig->Config(maxRecursionDepth);
   
   hr = device->CreateStateObject(rtPipeline, IID_PPV_ARGS(&dxrStateObject));
   assert(hr == S_OK);
   

}
