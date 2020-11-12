#pragma once
class siMesh;

class rtBlas
{
   std::map<siMesh*, UINT> geometryDescsAssociationMap;
   std::vector< D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs;
   D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS blasInputs;
   D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC blasDesc;

   ID3D12Resource* scratchData;
   ID3D12Resource* destData;
   D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo;
public:
   void AddMeshToGeometryDesc(siMesh* pMesh);
   void OnInit(ID3D12Device5* device, ID3D12GraphicsCommandList5* commandList);
};

