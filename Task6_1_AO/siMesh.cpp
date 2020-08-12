#include "siMesh.h"
#include "siCommandList.h"

void siMesh::initBuffer(ID3D12Device* device, const siCommandList& commandList)
{
   HRESULT hr;

   // Vertex buffer
   hr = device->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
      D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Buffer(vertices.size() * sizeof(siVertex)),
      D3D12_RESOURCE_STATE_COPY_DEST,
      nullptr,
      IID_PPV_ARGS(&vertexBuffer));
   assert(hr == S_OK);
   vertexBuffer.Get()->SetName(L"Vertex buffer default");

   hr = device->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
      D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Buffer(vertices.size() * sizeof(siVertex)),
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&vBufferUploadHeap));
   assert(hr == S_OK);
   vertexBuffer.Get()->SetName(L"Vertex buffer upload");

   commandList.updateSubresource(
      vertexBuffer.Get(),
      vBufferUploadHeap.Get(),
      {
         vertices.data(),
         static_cast<LONG_PTR>(vertices.size() * sizeof(siVertex)),
         static_cast<LONG_PTR>(vertices.size() * sizeof(siVertex))
      }
   );

   vertexBufferView = {
      vertexBuffer->GetGPUVirtualAddress(),
      static_cast<UINT>(vertices.size() * sizeof(siVertex)),
      sizeof(siVertex)};

   // Index buffer
   hr = device->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
      D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(DWORD)),
      D3D12_RESOURCE_STATE_COPY_DEST,
      nullptr,
      IID_PPV_ARGS(&indexBuffer));
   assert(hr == S_OK);
   vertexBuffer.Get()->SetName(L"Index buffer default");

   hr = device->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
      D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(DWORD)),
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&iBufferUploadHeap));
   assert(hr == S_OK);
   vertexBuffer.Get()->SetName(L"Index buffer upload");

   commandList.updateSubresource(
      indexBuffer.Get(),
      iBufferUploadHeap.Get(),
      {
         indices.data(),
         static_cast<LONG_PTR>(indices.size() * sizeof(DWORD)),
         static_cast<LONG_PTR>(indices.size() * sizeof(DWORD))
      });

   indexBufferView = {
      indexBuffer->GetGPUVirtualAddress(),
      static_cast<UINT>(indices.size() * sizeof(DWORD)),
      DXGI_FORMAT::DXGI_FORMAT_R32_UINT
   };
}
