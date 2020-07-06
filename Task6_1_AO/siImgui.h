#pragma once
class siWindow;

class siImgui
{
   XMFLOAT3* camPos;
   XMFLOAT3* camTarget;
public:
   siImgui();

   void onInit();
   void onInitWindow(HWND window);
   void onInitRenderer(ID3D12Device* device, uint32_t bufferCount, ID3D12DescriptorHeap* heap,
                              std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> handles);
   void onUpdate();
   void onRender(ID3D12GraphicsCommandList* commandList);

   void bindVariables(void* cameraPos, void* cameraTarget);
};
