#pragma once
#include "structures.h"
class siCommandList;
class siSceneLoader;

class siMesh
{
   friend class siSceneLoader;
   std::vector<DWORD> indices;
   std::vector<siVertex> vertices;
   std::string diffuseMap;
   std::string normalMap;
   std::string materialMap;
   ComPtr<ID3D12Resource> vertexBuffer;
   ComPtr<ID3D12Resource> indexBuffer;
   ComPtr<ID3D12Resource> vBufferUploadHeap;
   ComPtr<ID3D12Resource> iBufferUploadHeap;
   D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
   D3D12_INDEX_BUFFER_VIEW indexBufferView;

   void initBuffer(ID3D12Device* device, const siCommandList& commandList);
public:
   [[nodiscard]] UINT getIndexCount() const { return static_cast<UINT>(indices.size()); }
   [[nodiscard]] const D3D12_VERTEX_BUFFER_VIEW& getVertexBufferView() const { return vertexBufferView; }
   [[nodiscard]] const D3D12_INDEX_BUFFER_VIEW& getIndexBufferView() const { return indexBufferView; }
   [[nodiscard]] const std::string& getDiffuseMap() const { return diffuseMap; }
};
