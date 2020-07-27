#pragma once
#include "cacao.h"
class siWindow;
class siRenderer;

class siImgui
{
   siRenderer* renderer;
public:
   siImgui();

   void onInit();
   void onInitWindow(HWND window);
   void onInitRenderer(ID3D12Device* device, uint32_t bufferCount, ID3D12DescriptorHeap* heap,
                       std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> handles, siRenderer* renderer);
   void onUpdate();
   void onRender(ID3D12GraphicsCommandList* commandList);
};
