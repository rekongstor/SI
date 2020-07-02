#pragma once
class siWindow;

class siSwapChain
{
   ComPtr<IDXGISwapChain3> swapChain;
   ComPtr<ID3D12Resource> renderTargets[maxFrameBufferCount];
public:
   void onInit(siWindow* window, DXGI_SAMPLE_DESC sampleDesc, uint32_t bufferCount, IDXGIFactory4* factory,
               ID3D12CommandQueue* commandQueue);
   [[nodiscard]] ComPtr<ID3D12Resource>& getBuffer(uint32_t frame) { return renderTargets[frame]; }
   ComPtr<IDXGISwapChain3>& get() { return swapChain; }
};
