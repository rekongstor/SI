#include "SceneConstBuf.h"
#include "rnd_Dx12.h"

ConstBufInitializer<SceneConstBuf> SCENE_CB(L"SceneConstBuffer0");

void SceneConstBuf::OnInit(LPCWSTR name)
{
   // Create the constant buffer memory and map the CPU and GPU addresses
   const D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

   // Allocate one constant buffer per frame, since it gets updated every frame.
   size_t cbSize = FRAME_COUNT * AlignConst(sizeof(SceneConstantBuffer), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
   const D3D12_RESOURCE_DESC constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);

   ThrowIfFailed(renderer->device->CreateCommittedResource(
      &uploadHeapProperties,
      D3D12_HEAP_FLAG_NONE,
      &constantBufferDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&buffer)));
   buffer->SetName(name);

   ThrowIfFailed(buffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedData)));

   // Map the constant buffer and cache its heap pointers.
   // We don't unmap this until the app closes. Keeping buffer mapped for the lifetime of the resource is okay.
}

void SceneConstBuf::Update()
{
   SceneConstantBuffer* buf = this;
   memcpy(&mappedData[renderer->currentFrame].buffer, buf, sizeof(SceneConstantBuffer));
}
