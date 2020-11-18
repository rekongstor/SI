#include "rtAccelerationStructures.h"

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
}
