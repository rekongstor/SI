#pragma once
#include "siRootSignature.h"

class siCommandQueue;
class siCommandAllocator;
class siMesh;

class rtAccelerationStructures
{
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
   ComPtr<ID3D12StateObjectProperties> stateObjectProperties;

   siRootSignature rtLocalRootSignature;
   siRootSignature rtGlobalRootSignature;

public:
   void AddMeshToGeometryDesc(siMesh* pMesh);
   void OnInit(ID3D12Device5* device, ID3D12GraphicsCommandList5* commandList);
};

