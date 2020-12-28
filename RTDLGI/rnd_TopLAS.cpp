#include "rnd_TopLAS.h"

#include "rnd_Dx12.h"

inline void AllocateUploadBuffer(void* pData, UINT64 datasize, ID3D12Resource** ppResource, LPCWSTR resourceName = L"")
{
   auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
   auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(datasize);
   ThrowIfFailed(renderer->Device()->CreateCommittedResource(
      &uploadHeapProperties,
      D3D12_HEAP_FLAG_NONE,
      &bufferDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(ppResource)));
   (*ppResource)->SetName(resourceName);

   void* pMappedData;
   (*ppResource)->Map(0, nullptr, &pMappedData);
   memcpy(pMappedData, pData, datasize);
   (*ppResource)->Unmap(0, nullptr);
}

void rnd_TopLAS::OnInit(rnd_Scene* scene, LPCWSTR name)
{
   int totalInstances = 0;

   // for each instance
   std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescData;
   for (auto& inst : scene->instances)
   {
      D3D12_RAYTRACING_INSTANCE_DESC &instanceDesc = instanceDescData.emplace_back();
      for (int tr = 0; tr < 12; ++tr)
      {
         instanceDesc.Transform[tr / 4][tr % 4] = inst.second.instanceData.worldMat[tr / 4].m128_f32[tr % 4];
      }
      instanceDesc.InstanceMask = 1;
      instanceDesc.InstanceID = totalInstances++;
      instanceDesc.AccelerationStructure = inst.first->bottomLas.buffer->GetGPUVirtualAddress();
   }
   AllocateUploadBuffer(instanceDescData.data(), instanceDescData.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC), &instanceData, FormatWStr(L"InstanceDescs %s", name));

   // Get required sizes for an acceleration structure.
   D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

   D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
   D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& topLevelInputs = topLevelBuildDesc.Inputs;
   topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
   topLevelInputs.Flags = buildFlags;
   topLevelInputs.NumDescs = totalInstances; // number of instances
   topLevelInputs.pGeometryDescs = nullptr;
   topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

   D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
   renderer->dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
   ThrowIfFalse(topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

   ComPtr<ID3D12Resource> scratchResource;
   renderer->AllocateUAVBuffer(topLevelPrebuildInfo.ScratchDataSizeInBytes, &scratchResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"TlasScratchResource");

   renderer->AllocateUAVBuffer(topLevelPrebuildInfo.ResultDataMaxSizeInBytes, &buffer, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, L"TLAS");

   topLevelBuildDesc.DestAccelerationStructureData = buffer->GetGPUVirtualAddress();
   topLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
   topLevelBuildDesc.Inputs.InstanceDescs = instanceData->GetGPUVirtualAddress();

   renderer->dxrCommandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);

   this->state = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

   renderer->ExecuteCommandList();
   renderer->WaitForGpu();
}
