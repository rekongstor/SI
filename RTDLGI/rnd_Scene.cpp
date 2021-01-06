#include "rnd_Scene.h"
#include "rnd_Dx12.h"
#include <fstream>
#include "../3rd_party/fbx/src/ofbx.h"
#include <random>

void rnd_Scene::OnInit(LPCWSTR filename)
{
   std::ifstream fbx(filename, std::ios::binary);
   std::vector<char> bytes;
   fbx.seekg(0, std::ios::end);
   bytes.resize(fbx.tellg());
   fbx.seekg(0, std::ios::beg);
   fbx.read(bytes.data(), bytes.size());

   int totalInstances = 0;

   auto scene = ofbx::load((ofbx::u8*)bytes.data(), bytes.size(), (ofbx::u64)ofbx::LoadFlags::TRIANGULATE);
   meshes.reserve(scene->getMeshCount());
   for (int m = 0; m < scene->getMeshCount(); ++m)
   {
      auto mesh = scene->getMesh(m)->getGeometry();
      rnd_Mesh& dstMesh = meshes.emplace_back();
      rnd_Instance& instData = instances[&dstMesh];

      ++totalInstances;

      ofbx::Matrix tr = scene->getMesh(m)->getGlobalTransform();
      XMFLOAT3X4 wMatr;
      for (int t = 0; t < 12; ++t)
      {
         int i = t % 4;
         int j = t / 4;
         instData.instanceData.worldMat[j].m128_f32[i] = tr.m[i * 4 + j];
         wMatr.m[j][i] = tr.m[i * 4 + j];
      }
      if (m == 4)
      {
         float sh = -0.4 + 0.1 * 7;
         wMatr.m[0][3] += sh;
         //wMatr.m[1][3] += 0.0;
         //wMatr.m[2][3] += 0.0;
         instData.instanceData.worldMat[0].m128_f32[3] += sh;
      }

      XMMATRIX wmatrInv = XMMatrixInverse(nullptr, XMLoadFloat3x4(&wMatr));
      XMStoreFloat3x4(&wMatr, wmatrInv);
      memcpy(instData.instanceData.worldMatInv->m128_f32, wMatr.m, sizeof(float) * 12);

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
      dstMesh.OnInit(verticesData, indicesData, WStrFromStr(mesh->name).c_str());
   }

   topLayerAS.OnInit(this, filename);
   instancesDataBuffer[0].OnInit(totalInstances, sizeof(Instance), FormatWStr(L"Instance structured buffer [%s:%d]", filename, 0));
   instancesDataBuffer[1].OnInit(totalInstances, sizeof(Instance), FormatWStr(L"Instance structured buffer [%s:%d]", filename, 1));

   for (auto& inst : instances)
   {
      auto sbPair = instancesDataBuffer[0].AddBuffer(inst.second.instanceData);
      instancesDataBuffer[1].AddBuffer(inst.second.instanceData);
      inst.second.instIdx = sbPair;
   }
}

float random()
{
   static std::mt19937 rng;
   static std::uniform_real_distribution<> dist;

   return dist(rng);
}

void rnd_Scene::OnUpdate()
{
   for (auto& inst : instances) {
      if (renderer->counter >= 0 && inst.first != &meshes[1]) {
         XMVECTOR transl{ random() * 1.5 - 1, random() * 1.5 - 1, random() * 1.5 - 1};
         float uS = random() * 0.3 + 0.3;
         XMVECTOR scale{ uS,uS,uS };
         XMVECTOR rot{ random() * 2 * M_PI, random() * M_PI, random() * M_PI };
         XMMATRIX worldMat = XMMatrixRotationRollPitchYawFromVector(rot) * XMMatrixScalingFromVector(scale) * XMMatrixTranslationFromVector(transl);
         XMStoreFloat3x4((XMFLOAT3X4*)(void*)inst.second.instanceData.worldMat, worldMat);
         XMStoreFloat3x4((XMFLOAT3X4*)(void*)inst.second.instanceData.worldMatInv, XMMatrixInverse(nullptr, worldMat));
      }

      instancesDataBuffer[0].UpdateBuffer(inst.second.instanceData, inst.second.instIdx);
      instancesDataBuffer[1].UpdateBuffer(inst.second.instanceData, inst.second.instIdx);
   }
}
