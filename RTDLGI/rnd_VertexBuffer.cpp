#include "rnd_VertexBuffer.h"
#include "rnd_Dx12.h"

void rnd_VertexBuffer::OnInit(void* srcData, UINT64 sizeInBytes, int sizeOfElement, LPCWSTR name /*= L""*/)
{
   ID3D12Resource* uploadBuffer;
   auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
   auto bufferDesc(CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes, D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE));
   ThrowIfFailed(renderer->device->CreateCommittedResource(
      &heapProperties,
      D3D12_HEAP_FLAG_NONE,
      &bufferDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&uploadBuffer)));
   uploadBuffer->SetName(FormatWStr(L"[UploadBuf] %s", name));

   heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
   bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes);
   ThrowIfFailed(renderer->device->CreateCommittedResource(
      &heapProperties,
      D3D12_HEAP_FLAG_NONE,
      &bufferDesc,
      D3D12_RESOURCE_STATE_COPY_DEST,
      nullptr,
      IID_PPV_ARGS(&buffer)));
   buffer->SetName(name);

   this->format = DXGI_FORMAT_UNKNOWN;
   this->state = D3D12_RESOURCE_STATE_COPY_DEST;

   D3D12_SUBRESOURCE_DATA subresourceData{ srcData, sizeInBytes, sizeInBytes };
   UpdateSubresources(renderer->CommandListCopy(), buffer.Get(), uploadBuffer, 0, 0, 1, &subresourceData);

   renderer->AddUploadBuffer(uploadBuffer, buffer); // we won't release command buffer until all resources are loaded

   vertexBufferView.SizeInBytes = sizeInBytes;
   vertexBufferView.BufferLocation = buffer->GetGPUVirtualAddress();
   vertexBufferView.StrideInBytes = sizeOfElement;
}

void rnd_VertexBuffer::CreateSrv()
{
   // SRV
   D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
   srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
   srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
   srvDesc.Buffer.NumElements = vertexBufferView.SizeInBytes / vertexBufferView.StrideInBytes;

   srvDesc.Format = DXGI_FORMAT_UNKNOWN;
   srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
   srvDesc.Buffer.StructureByteStride = vertexBufferView.StrideInBytes;

   auto descHandle = renderer->GetCbvSrvUavHandle();
   srvHandle.first = descHandle.first;
   srvHandle.second = descHandle.second;

   renderer->device->CreateShaderResourceView(buffer.Get(), &srvDesc, descHandle.first);
}