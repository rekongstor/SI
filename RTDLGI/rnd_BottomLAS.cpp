#include "rnd_BottomLAS.h"

#include "rnd_Dx12.h"
#include "rnd_IndexBuffer.h"
#include "rnd_VertexBuffer.h"
#include "HlslCompat.h"

void rnd_BottomLAS::OnInit(rnd_IndexBuffer& indexBuffer, rnd_VertexBuffer& vertexBuffer, LPCWSTR name)
{
   D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc {};
   geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
   geometryDesc.Triangles.IndexBuffer = indexBuffer.buffer->GetGPUVirtualAddress();
   geometryDesc.Triangles.IndexCount = static_cast<UINT>(indexBuffer.buffer->GetDesc().Width) / sizeof(Index);
   geometryDesc.Triangles.IndexFormat = indexBuffer.format;
   geometryDesc.Triangles.Transform3x4 = 0;
   geometryDesc.Triangles.VertexFormat = vertexBuffer.format;
   geometryDesc.Triangles.VertexCount = static_cast<UINT>(vertexBuffer.buffer->GetDesc().Width) / sizeof(Vertex);
   geometryDesc.Triangles.VertexBuffer.StartAddress = vertexBuffer.buffer->GetGPUVirtualAddress();
   geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
   geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE; // Note: When rays encounter opaque geometry an any hit shader will not be executed whether it is present or not.

   // Get required sizes for an acceleration structure.
   D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

   D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
   D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& bottomLevelInputs = bottomLevelBuildDesc.Inputs;
   bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
   bottomLevelInputs.Flags = buildFlags;
   bottomLevelInputs.NumDescs = 1;
   bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
   bottomLevelInputs.pGeometryDescs = &geometryDesc;

   D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
   renderer->dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
   ThrowIfFalse(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

   ComPtr<ID3D12Resource> scratchResource;
   renderer->AllocateUAVBuffer(bottomLevelPrebuildInfo.ScratchDataSizeInBytes, &scratchResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"BlasScratchResource");

   renderer->AllocateUAVBuffer(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, &buffer, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, FormatWStr(L"[BLAS] %s", name));

   bottomLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
   bottomLevelBuildDesc.DestAccelerationStructureData = buffer->GetGPUVirtualAddress();

   renderer->dxrCommandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);

   auto c = CD3DX12_RESOURCE_BARRIER::UAV(buffer.Get());
   renderer->CommandList()->ResourceBarrier(1, &c);

   this->state = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

   renderer->ExecuteCommandList();
   renderer->WaitForGpu();
}
