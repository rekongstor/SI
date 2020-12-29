#include "SceneConstBuf.h"
#include "rnd_Dx12.h"
#include "core_Window.h"

ConstBufInitializer<SceneConstBuf> SCENE_CB(L"SceneConstBuffer0");

void SceneConstBuf::OnInit(LPCWSTR name)
{
   // Create the constant buffer memory and map the CPU and GPU addresses
   const D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

   // Allocate one constant buffer per frame, since it gets updated every frame.
   size_t cbSize = FRAME_COUNT * AlignConst(sizeof(SceneConstantBuffer), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
   D3D12_RESOURCE_DESC constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);

   ThrowIfFailed(renderer->Device()->CreateCommittedResource(
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
   float x = renderer->camPos.x;
   float y = renderer->camPos.y;
   float z = renderer->camPos.z;
   float pitch = renderer->camDir.x;
   float yaw = renderer->camDir.y;

   XMMATRIX view = XMMatrixInverse(nullptr, XMMatrixRotationRollPitchYaw(pitch, -yaw, 0.f) * XMMatrixTranslation(x, y, z));
   XMMATRIX proj = XMMatrixPerspectiveFovRH(XMConvertToRadians(renderer->fovAngleY), (float)window->width / (float)window->height, 0.01f, 10000.0f);

   viewProj = view * proj;
   viewProjInv = XMMatrixInverse(nullptr, viewProj);

   cameraPosition = renderer->camPos;
   lightPosition = renderer->lightPosition;
   lightDirection = renderer->lightDirection;
   lightAmbientColor = renderer->lightAmbientColor;
   lightDiffuseColor = renderer->lightDiffuseColor;
   screenData = { (float)window->width, (float)window->height, 1.f / window->width, 1.f / window->height };

   static int cntr = 0;
   counter.x = float(cntr++);

   memcpy(&mappedData[renderer->currentFrame].buffer, buf, sizeof(SceneConstantBuffer));
}
