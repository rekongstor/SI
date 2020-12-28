#include "rnd_StructuredBuffer.h"
#include "rnd_Dx12.h"

void rnd_StructuredBuffer::OnInit(int reservedElements, int sizeOfElement, LPCWSTR name)
{
   auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
   auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(reservedElements * sizeOfElement);
   ThrowIfFailed(renderer->Device()->CreateCommittedResource(
      &heapProps,
      D3D12_HEAP_FLAG_NONE,
      &resDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr, IID_PPV_ARGS(&buffer)));
   buffer->SetName(name);

   this->sizeOfElement = sizeOfElement;
   ThrowIfFailed(buffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedData)));
}
