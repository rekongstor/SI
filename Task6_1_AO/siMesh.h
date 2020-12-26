#pragma once
#include "siTexture.h"
#include "structures.h"
class siCommandList;
class siSceneLoader;

class siMesh
{
   friend class siSceneLoader;
   std::vector<DWORD> indices;
   std::vector<siVertex> vertices;
   siTexture diffuseMapTexture;
   siTexture normalMapTexture;
   siTexture materialMapTexture;
   ComPtr<ID3D12Resource> vertexBuffer;
   ComPtr<ID3D12Resource> indexBuffer;
   ComPtr<ID3D12Resource> vBufferUploadHeap;
   ComPtr<ID3D12Resource> iBufferUploadHeap;
   D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
   D3D12_INDEX_BUFFER_VIEW indexBufferView;


   void initBuffer(ID3D12Device* device, const siCommandList& commandList);
public:
   XMFLOAT4X4 worldPos;
   [[nodiscard]] UINT getIndexCount() const { return static_cast<UINT>(indices.size()); }
   [[nodiscard]] UINT getVertexCount() const { return static_cast<UINT>(vertices.size()); }

   [[nodiscard]] const D3D12_VERTEX_BUFFER_VIEW& getVertexBufferView() const { return vertexBufferView; }
   [[nodiscard]] const D3D12_INDEX_BUFFER_VIEW& getIndexBufferView() const { return indexBufferView; }
   [[nodiscard]] const ComPtr<ID3D12Resource>& getIndexBuffer() const { return indexBuffer; }
   [[nodiscard]] const ComPtr<ID3D12Resource>& getVertexBuffer() const { return vertexBuffer; }
   [[nodiscard]] const siTexture& getDiffuseMap() const { return diffuseMapTexture; }
};
