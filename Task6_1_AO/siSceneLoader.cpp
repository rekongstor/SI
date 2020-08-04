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

      dstMesh.diffuseMap = mesh.MeshMaterial.map_Kd;
      dstMesh.materialMap = mesh.MeshMaterial.map_Ka;
      dstMesh.normalMap = mesh.MeshMaterial.map_bump;
      {
         auto& tex = textures[dstMesh.diffuseMap];
         //if (tex.getState() != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
         {
            tex.initFromFile(device, dstMesh.diffuseMap, commandList);
            tex.createSrv(device, descriptorMgr);
         }
      }
      {
         auto& tex = textures[dstMesh.materialMap];
         //if (tex.getState() != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
         {
            tex.initFromFile(device, dstMesh.materialMap, commandList);
            tex.createSrv(device, descriptorMgr);
         }
      }
      {
         auto& tex = textures[dstMesh.normalMap];
         //if (tex.getState() != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
         {
            tex.initFromFile(device, dstMesh.normalMap, commandList);
            tex.createSrv(device, descriptorMgr);
         }
      }
   }
}
