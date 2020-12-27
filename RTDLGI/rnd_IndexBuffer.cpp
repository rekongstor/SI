#include "rnd_IndexBuffer.h"
#include "rnd_Dx12.h"

void rnd_IndexBuffer::OnInit(std::vector<char>& data, int sizeOfElement, LPCWSTR name /*= L""*/)
{
   cpuBuffer = std::move(data);

   ID3D12Resource* uploadBuffer;
   auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
   auto bufferDesc(CD3DX12_RESOURCE_DESC::Buffer(cpuBuffer.size(), D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE));
   ThrowIfFailed(renderer->Device()->CreateCommittedResource(
      &heapProperties,
      D3D12_HEAP_FLAG_NONE,
      &bufferDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&uploadBuffer)));
   uploadBuffer->SetName(FormatWStr(L"[UploadBuf-IndexBuf] %s", name));

   heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
   bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(cpuBuffer.size());
   ThrowIfFailed(renderer->Device()->CreateCommittedResource(
      &heapProperties,
      D3D12_HEAP_FLAG_NONE,
      &bufferDesc,
      D3D12_RESOURCE_STATE_COPY_DEST,
      nullptr,
      IID_PPV_ARGS(&buffer)));
   buffer->SetName(FormatWStr(L"[IndexBuf] %s", name));

   D3D12_SUBRESOURCE_DATA subresourceData{ cpuBuffer.data(), cpuBuffer.size(), cpuBuffer.size() };
   UpdateSubresources(renderer->CommandListCopy(), buffer.Get(), uploadBuffer, 0, 0, 1, &subresourceData);

   switch (sizeOfElement)
   {
   case 2:
      this->format = DXGI_FORMAT_R16_UINT;
      break;
   case 4:
      this->format = DXGI_FORMAT_R32_UINT;
      break;
   default:
      ThrowMsg(L"Invalid index format");
   }
   this->state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

   renderer->AddUploadBuffer(uploadBuffer, this); // we won't release command buffer until all resources are loaded

   indexBufferView.SizeInBytes = cpuBuffer.size();
   indexBufferView.Format = this->format;
   indexBufferView.BufferLocation = buffer->GetGPUVirtualAddress();
}

void rnd_IndexBuffer::CreateSrv()
{
   // SRV
   D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
   srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
   srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
   srvDesc.Buffer.NumElements = indexBufferView.SizeInBytes / sizeof(UINT32);

   srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
   srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
   srvDesc.Buffer.StructureByteStride = 0;

   auto descHandle = renderer->GetCbvSrvUavHandle();
   srvHandle.first = descHandle.first;
   srvHandle.second = descHandle.second;

   renderer->Device()->CreateShaderResourceView(buffer.Get(), &srvDesc, descHandle.first);
}
