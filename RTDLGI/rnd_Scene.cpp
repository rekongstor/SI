#include "rnd_Scene.h"
#include <fstream>
#include "../3rd_party/fbx/src/ofbx.h"

void rnd_Scene::OnInit(LPCSTR filename)
{
   std::ifstream fbx(filename, std::ios::binary);
   std::vector<char> bytes;
   fbx.seekg(0, std::ios::end);
   bytes.resize(fbx.tellg());
   fbx.seekg(0, std::ios::beg);
   fbx.read(bytes.data(), bytes.size());


   auto scene = ofbx::load((ofbx::u8*)bytes.data(), bytes.size(), (ofbx::u64)ofbx::LoadFlags::TRIANGULATE);
   meshes.reserve(scene->getMeshCount());
   for (int m = 0; m < scene->getMeshCount(); ++m)
   {
      auto mesh = scene->getMesh(m)->getGeometry();
      rnd_Mesh& dstMesh = meshes.emplace_back();
      rnd_Instance& instData = instances[&dstMesh];

      ofbx::Matrix tr = scene->getMesh(m)->getGlobalTransform();
      for (int t = 0; t < 16; ++t)
      {
         instData.instanceData.worldMat.r[t / 4].m128_f32[t % 4] = tr.m[t];
      }

      const int* faceIndices = mesh->getFaceIndices();
      int indicesCount = mesh->getIndexCount();
      int verticesCount = mesh->getVertexCount();
      std::vector<char> indicesData(indicesCount * sizeof(Index));
      std::vector<char> verticesData(verticesCount * sizeof(Vertex));
      Index* indices = (Index*)indicesData.data();
      Vertex* vertices = (Vertex*)verticesData.data();

      for (int i = 0; i < indicesCount; ++i) {
         int idx = (faceIndices[i] < 0) ? -faceIndices[i] : (faceIndices[i] + 1);
         int vertex_idx = idx;
         indices[i] = vertex_idx - 1;
      }
      for (int v = 0; v < verticesCount; ++v) {
         auto pos = mesh->getVertices()[v];
         auto normal = mesh->getNormals()[v];
         auto uv = mesh->getUVs()[v];
         vertices[v].position = { (float)pos.x, (float)pos.y, (float)pos.z };
         vertices[v].normal = { (float)normal.x, (float)normal.y, (float)normal.z };
         vertices[v].uv = { (float)uv.x, (float)uv.y };
      }
      dstMesh.OnInit(verticesData, indicesData, std::wstring(&mesh->name[0], &mesh->name[strlen(mesh->name)]).c_str());
   }

   topLayerAS.OnInit(this);
}
