#include "rnd_Tensor.h"
#include "rnd_Dx12.h"

void rnd_InputTensor::AssignData(std::vector<char>& data)
{
   cpuBuffer = std::move(data);
}

void rnd_InputTensor::OnInit(UINT64 size, LPCWSTR name)
{
   ID3D12Resource* uploadBuffer;
   auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
   auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(size);
   renderer->Device()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));
   uploadBuffer->SetName(FormatWStr(L"[UploadBuf-Tensor] %s", name));

   heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
   resDesc = CD3DX12_RESOURCE_DESC::Buffer(size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
   renderer->Device()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&buffer));
   buffer->SetName(FormatWStr(L"[Tensor] %s", name));


   D3D12_SUBRESOURCE_DATA subresourceData{ cpuBuffer.data(), cpuBuffer.size(), cpuBuffer.size() };
   UpdateSubresources(renderer->CommandListCopy(), buffer.Get(), uploadBuffer, 0, 0, 1, &subresourceData);

   this->format = DXGI_FORMAT_R16_FLOAT;
   this->state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
   width = size;

   renderer->AddUploadBuffer(uploadBuffer, this); // we won't release command buffer until all resources are loaded
}

void rnd_DynamicTensor::OnInit(UINT64 size, LPCWSTR name)
{
   auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
   auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
   renderer->Device()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&buffer));
   buffer->SetName(FormatWStr(L"[Tensor] %s", name));

   width = size;
   state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
}

void rnd_Tensor::CreateUav()
{
   uavHandle = renderer->GetCbvSrvUavHandle();
   D3D12_UNORDERED_ACCESS_VIEW_DESC desc;
   ZeroMemory(&desc, sizeof(desc));
   desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
   desc.Format = DXGI_FORMAT_UNKNOWN;
   desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
   desc.Buffer.CounterOffsetInBytes = 0;
   desc.Buffer.FirstElement = 0;
   desc.Buffer.NumElements = width / 2;
   desc.Buffer.StructureByteStride = 2;

   renderer->Device()->CreateUnorderedAccessView(Buffer()->buffer.Get(), nullptr, &desc, uavHandle.first);
   ThrowIfFailed(renderer->Device()->GetDeviceRemovedReason());
}

