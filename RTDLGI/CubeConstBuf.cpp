#include "CubeConstBuf.h"
#include "rnd_Dx12.h"

const ConstBufInitializer<CubeConstBuf> cubeCbName(L"CubeConstBuffer");

void CubeConstBuf::OnInit(LPCWSTR name)
{
   // Create the constant buffer memory and map the CPU and GPU addresses
   const D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

   // Allocate one constant buffer per frame, since it gets updated every frame.
   size_t cbSize = AlignConst(sizeof(CubeConstantBuffer), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
   const D3D12_RESOURCE_DESC constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);

   ThrowIfFailed(renderer->device->CreateCommittedResource(
      &uploadHeapProperties,
      D3D12_HEAP_FLAG_NONE,
      &constantBufferDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&buffer)));
   buffer->SetName(name);
}

void CubeConstBuf::Update()
{
   CubeConstantBuffer* buf = this;
   memcpy(&mappedData, buf, sizeof(CubeConstantBuffer));
}
