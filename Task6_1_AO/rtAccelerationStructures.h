#pragma once
#include "siRootSignature.h"
#include "rt.h"

class siCommandQueue;
class siCommandAllocator;
class siMesh;

class rtAccelerationStructures
{
public:
   std::map<siMesh*, UINT> geometryDescsAssociationMap;
   std::vector< D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs;

   D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS blasInputs;
   D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC blasDesc;

   D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS tlasInputs;
   D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC tlasDesc;

   ComPtr<ID3D12Resource> destDataBlas;
   ComPtr<ID3D12Resource> destDataTlas;
   ComPtr<ID3D12GraphicsCommandList4> dxrCommandList;

   ComPtr<ID3D12Resource> instanceDescs;
   ComPtr<ID3D12Resource> scratchData;

   ComPtr<ID3D12StateObject> dxrStateObject;

   siRootSignature rtLocalRootSignature;
   siRootSignature rtGlobalRootSignature;

   RayGenConstantBuffer rayGenCb;

   struct ShaderTable
   {
      uint8_t* mappedShaderRecords;
      UINT shaderRecordSize;

      struct ShaderRecord
      {
         struct PointerWithSize
         {
            void* ptr;
            UINT size;
         } shaderId, localRootArgs;
      };
      std::vector<ShaderRecord> shaderRecords;

      ID3D12Resource* m_resource;
      void Allocate(ID3D12Device* device, UINT bufferSize, LPCWSTR resourceName = nullptr)
      {
         auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

         auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
         HRESULT hr = device->CreateCommittedResource(
            &uploadHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_resource));
         assert(hr == S_OK);
         m_resource->SetName(resourceName);
      }

      void MapCpuWriteOnly()
      {
         uint8_t* mappedData;
         // We don't unmap this until the app closes. Keeping buffer mapped for the lifetime of the resource is okay.
         CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
         HRESULT hr = m_resource->Map(0, &readRange, reinterpret_cast<void**>(&mappedData));
         assert(hr == S_OK);
         mappedShaderRecords = mappedData;
      }
   };

   ComPtr<ID3D12Resource> m_missShaderTable;
   ComPtr<ID3D12Resource> m_hitGroupShaderTable;
   ComPtr<ID3D12Resource> m_rayGenShaderTable;


   D3D12_RAYTRACING_INSTANCE_DESC instanceDesc{};
   CD3DX12_STATE_OBJECT_DESC rtPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };
   ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
   struct RootArguments
   {
      RayGenConstantBuffer cb;
   } rootArguments;
   ShaderTable rayGenShaderTable;
   ShaderTable missShaderTable;
   ShaderTable hitGroupShaderTable;
public:
   void AddMeshToGeometryDesc(siMesh* pMesh);
   void OnInit(ID3D12Device5* device, ID3D12GraphicsCommandList5* commandList);
};

