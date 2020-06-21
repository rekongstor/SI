#pragma once
#include "stdafx.h"
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <map>
#include "Camera.h"
#include "Instance.h"
#include "Mesh.h"

struct cbPerFrame
{
   XMFLOAT4 direction;
   XMFLOAT4 color;
   XMFLOAT4X4 vpMatrix;
   XMFLOAT4 ambient;
   XMFLOAT4 camPos;
};

struct instanceData
{
   XMFLOAT4X4 wMatrix;
   XMFLOAT4 material;
};

struct Texture
{
   std::vector<BYTE> pixels;
   D3D12_RESOURCE_DESC desc;
   UINT64 textureHeapSize;
   int bytesPerRow;
   ID3D12Resource* textureBuffer;
   ID3D12Resource* textureUploadHeap;
};

#define CB_ALIGN(struct_)  ((sizeof(struct_) + 255) & ~255)
#define MUL_ALIGN(struct_)  (CB_ALIGN(struct_) >> 8)

class Window;

class Dx12Renderer
{
   Window* window;
   uint32_t frameBufferCount;
   static const uint32_t maxFrameBufferCount = 3;
   const D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
   bool active = true;

   ID3D12Device* device;
   IDXGISwapChain3* swapChain;
   ID3D12CommandQueue* commandQueue;
   ID3D12DescriptorHeap* rtvDescriptorHeap; // non-shader visible
   ID3D12DescriptorHeap* srvDescriptorHeap; // non-shader visible
   ID3D12Resource* renderTargets[maxFrameBufferCount];


   ID3D12Resource* depthStencilBuffer;
   ID3D12DescriptorHeap* dsDescriptorHeap;
   ID3D12CommandAllocator* commandAllocator[maxFrameBufferCount];
   ID3D12GraphicsCommandList* commandList;

   ID3D12Fence* fence[maxFrameBufferCount];
   HANDLE fenceEvent;
   UINT64 fenceValue[maxFrameBufferCount];


   ID3D12PipelineState* pipelineStateObject;
   ID3D12RootSignature* rootSignature;
   D3D12_VIEWPORT viewport;
   D3D12_RECT scissorRect;
   ID3D12Resource* vertexBuffer;
   D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
   ID3D12Resource* indexBuffer;
   D3D12_INDEX_BUFFER_VIEW indexBufferView;

   ID3D12Resource* instanceBuffer;
   UINT8* instanceDataGPUAddress;

   ID3D12Resource* constantBufferUploadHeaps[maxFrameBufferCount];
   UINT8* cbvGPUAddress[maxFrameBufferCount];

   Camera camera;

   std::vector<Mesh> meshes;
   std::map<Mesh*, std::vector<Instance>> instances;

   Texture albedo;
   Texture metallic;
   Texture roughness;

   void Update();
   void UpdatePipeline();
   void Render();
   void Cleanup();
   void WaitForPreviousFrame();

   void LoadTexture(LPCTSTR filename, Texture& tex);

   uint32_t currentFrame;
   uint32_t rtvDescriptorSize;
   uint32_t svDescriptorSize;
public:
   Dx12Renderer(Window* window, uint32_t frameBufferCount);

   [[nodiscard]] bool isActive() const;

   void OnInit();
   void OnUpdate();
   void OnDestroy();
};
