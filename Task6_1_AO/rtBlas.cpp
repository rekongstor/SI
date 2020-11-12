#include "rtBlas.h"
#include "siMesh.h"

void rtBlas::AddMeshToGeometryDesc(siMesh* pMesh)
{
   geometryDescsAssociationMap[pMesh] = geometryDescs.size();
   geometryDescs.push_back({});
   auto& geomDesc = geometryDescs.back();

   geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;

   geomDesc.Triangles.IndexBuffer = pMesh->getIndexBufferView().BufferLocation;
   geomDesc.Triangles.IndexBuffer = pMesh->getIndexCount();
   geomDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;

   geomDesc.Triangles.VertexBuffer = { pMesh->getVertexBufferView().BufferLocation, pMesh->getVertexBufferView().StrideInBytes };
   geomDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
}

void rtBlas::OnInit(ID3D12Device5* device, ID3D12GraphicsCommandList5* commandList)
{
   blasInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT::D3D12_ELEMENTS_LAYOUT_ARRAY;
   blasInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
   blasInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
   blasInputs.pGeometryDescs = geometryDescs.data();
   blasInputs.NumDescs = 1;

   device->GetRaytracingAccelerationStructurePrebuildInfo(&blasInputs, &prebuildInfo);

   //device->CreateCommittedResource()

   blasDesc.Inputs = blasInputs;
   blasDesc.ScratchAccelerationStructureData = scratchData->GetGPUVirtualAddress();
   blasDesc.DestAccelerationStructureData = destData->GetGPUVirtualAddress();

   commandList->BuildRaytracingAccelerationStructure(&blasDesc, 0, nullptr);
}
