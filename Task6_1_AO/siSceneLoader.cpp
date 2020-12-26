#include "siSceneLoader.h"
#include "siMesh.h"
#include "../3rd_party/OBJ-Loader/Source/OBJ_Loader.h"
//#include "../3rd_party/fbx/src/miniz.h"
#include "../3rd_party/fbx/src/ofbx.h"

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


void siSceneLoader::loadScene(LPCSTR filename, std::map<int32_t, siMesh>& meshes,
   std::map<std::string, siTexture>& textures, ID3D12Device* device, siDescriptorMgr* descriptorMgr,
   const siCommandList& commandList)
{
   FILE* fp;
   fopen_s(&fp, filename, "rb");

   if (!fp) return;

   fseek(fp, 0, SEEK_END);
   long file_size = ftell(fp);
   fseek(fp, 0, SEEK_SET);
   auto* content = new ofbx::u8[file_size];
   fread(content, 1, file_size, fp);
   auto scene = ofbx::load((ofbx::u8*)content, file_size, (ofbx::u64)ofbx::LoadFlags::TRIANGULATE);
   fclose(fp);
   for (int m = 0; m < scene->getMeshCount(); ++m) {
      auto mesh = scene->getMesh(m)->getGeometry();
      siMesh& dstMesh = meshes[m];
      ofbx::Matrix tr = scene->getMesh(m)->getGlobalTransform();
      for (int t = 0; t < 16; ++t)
      {
         dstMesh.worldPos.m[t / 4][t % 4] = tr.m[t];
      }

      const int* faceIndices = mesh->getFaceIndices();
      int index_count = mesh->getIndexCount();
      dstMesh.indices.resize(index_count);
      bool new_face = true;
      for (int i = 0; i < index_count; ++i) {
         int idx = (faceIndices[i] < 0) ? -faceIndices[i] : (faceIndices[i] + 1);
         int vertex_idx = idx;
         dstMesh.indices[i] = vertex_idx - 1;
      }

      dstMesh.vertices.resize(mesh->getVertexCount());
      for (int v = 0; v < mesh->getVertexCount(); ++v) {
         auto pos = mesh->getVertices()[v];
         auto normal = mesh->getNormals()[v];
         auto uv = mesh->getUVs()[v];
         dstMesh.vertices[v].position = { (float)pos.x, (float)pos.y, (float)pos.z, 1 };
         dstMesh.vertices[v].normal = { (float)normal.x, (float)normal.y, (float)normal.z, 0 };
         dstMesh.vertices[v].uv = { (float)uv.x, (float)uv.y };
      }
      dstMesh.initBuffer(device, commandList);

      assert(scene->getMesh(m)->getMaterialCount() == 1);
      auto mat = scene->getMesh(m)->getMaterial(0);
      char name[4096];
      {
         auto texName = mat->getTexture(ofbx::Texture::TextureType::DIFFUSE)->getRelativeFileName();
         texName.toString(name);
         auto& tex = textures[name];
         if (tex.getState() != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
            tex.initFromFile(device, name, commandList);
         }
         dstMesh.diffuseMapTexture.initFromTexture(tex);
         dstMesh.diffuseMapTexture.createSrv(device, descriptorMgr);
      }
      {
         auto texName = mat->getTexture(ofbx::Texture::TextureType::DIFFUSE)->getRelativeFileName();
         texName.toString(name);
         auto& tex = textures[name];
         if (tex.getState() != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
            tex.initFromFile(device, name, commandList);
         }
         dstMesh.materialMapTexture.initFromTexture(tex);
         dstMesh.materialMapTexture.createSrv(device, descriptorMgr);
      }
      {
         auto texName = mat->getTexture(ofbx::Texture::TextureType::DIFFUSE)->getRelativeFileName();
         texName.toString(name);
         auto& tex = textures[name];
         if (tex.getState() != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
            tex.initFromFile(device, name, commandList);
         }
         dstMesh.normalMapTexture.initFromTexture(tex);
         dstMesh.normalMapTexture.createSrv(device, descriptorMgr);
      }
   }
   //
   //for (uint32_t m = 0; m < loader.LoadedMeshes.size(); ++m) {
   //   auto& mesh = loader.LoadedMeshes[m];
   //   auto& dstMesh = meshes[m];
   //   dstMesh.indices.resize(mesh.Indices.size());
   //   for (size_t i = 0; i < mesh.Indices.size(); ++i)
   //      dstMesh.indices[i] = mesh.Indices[i];
   //   dstMesh.vertices.resize(mesh.Vertices.size());
   //   for (size_t i = 0; i < mesh.Vertices.size(); ++i) {
   //      auto& v = mesh.Vertices[i];
   //      dstMesh.vertices[i] = {
   //         {v.Position.X, v.Position.Y, v.Position.Z, 1.f},
   //         {v.Normal.X, v.Normal.Y, v.Normal.Z, 0.f},
   //         {v.TextureCoordinate.X, v.TextureCoordinate.Y}
   //      };
   //   }
   //   dstMesh.initBuffer(device, commandList);
   //   {
   //      auto& tex = textures[mesh.MeshMaterial.map_Kd];
   //      if (tex.getState() != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
   //         tex.initFromFile(device, mesh.MeshMaterial.map_Kd, commandList);
   //      }
   //      dstMesh.diffuseMapTexture.initFromTexture(tex);
   //      dstMesh.diffuseMapTexture.createSrv(device, descriptorMgr);
   //   }
   //   {
   //      auto& tex = textures[mesh.MeshMaterial.map_Ka];
   //      if (tex.getState() != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
   //         tex.initFromFile(device, mesh.MeshMaterial.map_Ka, commandList);
   //      }
   //      dstMesh.materialMapTexture.initFromTexture(tex);
   //      dstMesh.materialMapTexture.createSrv(device, descriptorMgr);
   //   }
   //   {
   //      auto& tex = textures[mesh.MeshMaterial.map_bump];
   //      if (tex.getState() != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
   //         tex.initFromFile(device, mesh.MeshMaterial.map_bump, commandList);
   //      }
   //      dstMesh.normalMapTexture.initFromTexture(tex);
   //      dstMesh.normalMapTexture.createSrv(device, descriptorMgr);
   //   }
   //}
}
