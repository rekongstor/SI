#include "rnd_SwapChainMgr.h"
#include "rnd_TextureMgr.h"
#include "core_Window.h"

#pragma optimize("", off)
void rnd_SwapChainMgr::OnInit(IDXGIFactory4* factory, ID3D12CommandQueue* commandQueue, rnd_TextureMgr* textureMgr, core_Window* window, DXGI_FORMAT backBufferFormat, bool windowed)
{
   assert(textureMgr != NULL && window != NULL);
   assert(window->width > 0 && window->height > 0);

   swapChainFormat = backBufferFormat;

   DXGI_SWAP_CHAIN_DESC1 swapChainDesc {};
   swapChainDesc.Width = window->width;
   swapChainDesc.Height = window->height;
   swapChainDesc.Format = swapChainFormat;
   swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // TODO: Do I need this? Maybe I should make this UAV for DLSS?
   swapChainDesc.BufferCount = 2;
   swapChainDesc.SampleDesc.Count = 1;
   swapChainDesc.SampleDesc.Quality = 0;
   swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
   swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
   swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
   swapChainDesc.Flags = 0; // enable swapChain.Resize
   // TODO: DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING?

   DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsDesc {};
   fsDesc.Windowed = windowed;

   ComPtr<IDXGISwapChain1> swapChainTmp;
   bool prevIsFullscreen = window->IsFullscreen();
   if (prevIsFullscreen) {
      window->SetWindowZorderToTopMost(false);
   }

   ThrowIfFailed(factory->CreateSwapChainForHwnd(commandQueue, window->window, &swapChainDesc, &fsDesc, nullptr, &swapChainTmp));

   if (prevIsFullscreen) {
      window->SetWindowZorderToTopMost(true);
   }

   ThrowIfFailed(swapChainTmp.As(&swapChain));

   ID3D12Resource* backBuffer;
   ThrowIfFailed(swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));
   textureMgr->backBuffer[0].OnInit(backBuffer, backBufferFormat, L"BackBuffer_0");
   ThrowIfFailed(swapChain->GetBuffer(1, IID_PPV_ARGS(&backBuffer)));
   textureMgr->backBuffer[1].OnInit(backBuffer, backBufferFormat, L"BackBuffer_1");
}
