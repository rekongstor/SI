#pragma once
#include "structures.h"
class siWindow;

class siImgui
{
   float4* camPos;
   float4* camTarget;
   int* targetOutput;
   float4* lightColor;
   float4* ambientColor;
   int* targetArr;
public:
   siImgui();

   void onInit();
   void onInitWindow(HWND window);
   void onInitRenderer(ID3D12Device* device, uint32_t bufferCount, ID3D12DescriptorHeap* heap,
                       std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> handles);
   void onUpdate();
   void onRender(ID3D12GraphicsCommandList* commandList);
   void bindVariables(float4* cameraPos, float4* cameraTarget, int* targetOutput, float4* lightColor,
                      float4* ambientColor, int* targetArr);
};
