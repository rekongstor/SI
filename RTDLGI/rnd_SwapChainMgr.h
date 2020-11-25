#pragma once
class core_Window;
class rnd_TextureMgr;

class rnd_SwapChainMgr
{
public:
   void OnInit(IDXGIFactory4* factory, ID3D12CommandQueue* commandQueue, rnd_TextureMgr* textureMgr, core_Window* window, DXGI_FORMAT backBufferFormat, bool windowed);
   void ResizeBuffers(rnd_TextureMgr* textureMgr, core_Window* window); // TODO: Needs implementation

   ComPtr<IDXGISwapChain3> swapChain;
   DXGI_FORMAT swapChainFormat;
   int currentFrame;
};

