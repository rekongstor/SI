#include "siSceneLoader.h"
#include "siMesh.h"
#include "../3rd_party/OBJ-Loader/Source/OBJ_Loader.h"

void siSceneLoader::loadScene(LPCSTR filename, std::map<int32_t, siMesh>& meshes,
                              std::map<std::string, siTexture>& textures, ID3D12Device* device,
                              const siCommandList& commandList, siDescriptorMgr* descriptorMgr)
{
   objl::Loader loader;
   loader.LoadFile(filename);
   for (uint32_t m = 0; m < loader.LoadedMeshes.size(); ++m)
   {
      auto& mesh = loader.LoadedMeshes[m];
      auto& dstMesh = meshes[m];

      dstMesh.indices.resize(mesh.Indices.size());
      for (size_t i = 0; i < mesh.Indices.size(); ++i)
         dstMesh.indices[i] = mesh.Indices[i];
      dstMesh.vertices.resize(mesh.Vertices.size());
      for (size_t i = 0; i < mesh.Vertices.size(); ++i)
      {
         auto& v = mesh.Vertices[i];
         dstMesh.vertices[i] = {
            {v.Position.X, v.Position.Y, v.Position.Z, 1.f},
            {v.Normal.X, v.Normal.Y, v.Normal.Z, 0.f},
            {v.TextureCoordinate.X, v.TextureCoordinate.Y}
         };
      }
      dstMesh.initBuffer(device, commandList);

      {
         auto& tex = textures[mesh.MeshMaterial.map_Kd];
         if (tex.getState() != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
         {
            tex.initFromFile(device, mesh.MeshMaterial.map_Kd, commandList);
         }
         dstMesh.diffuseMapTexture.initFromTexture(tex);
         dstMesh.diffuseMapTexture.createSrv(device, descriptorMgr);
      }
      {
         auto& tex = textures[mesh.MeshMaterial.map_Ka];
         if (tex.getState() != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
         {
            tex.initFromFile(device, mesh.MeshMaterial.map_Ka, commandList);
         }
         dstMesh.materialMapTexture.initFromTexture(tex);
         dstMesh.materialMapTexture.createSrv(device, descriptorMgr);
      }
      {
         auto& tex = textures[mesh.MeshMaterial.map_bump];
         if (tex.getState() != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
         {
            tex.initFromFile(device, mesh.MeshMaterial.map_bump, commandList);
         }
         dstMesh.normalMapTexture.initFromTexture(tex);
         dstMesh.normalMapTexture.createSrv(device, descriptorMgr);
      }
   }
}
