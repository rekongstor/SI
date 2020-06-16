#pragma once
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <DirectXMath.h>

using namespace DirectX; // I know it's bad

struct Vertex
{
   XMFLOAT3 pos;
   XMFLOAT3 color;
};

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
   ID3D12DescriptorHeap* nsvDescriptorHeap; // non-shader visible
   ID3D12Resource* renderTargets[maxFrameBufferCount];
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

   void Update();
   void UpdatePipeline();
   void Render();
   void Cleanup();
   void WaitForPreviousFrame();

   uint32_t currentFrame;
   uint32_t rtvDescriptorSize;
public:
   Dx12Renderer(Window* window, uint32_t frameBufferCount);

   [[nodiscard]] bool isActive() const;

   void OnInit();
   void OnUpdate();
   void OnDestroy();
};

