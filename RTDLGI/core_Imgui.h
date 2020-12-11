#pragma once

class core_Window;

class core_Imgui
{
public:
   void OnInit();
   void InitWindow(core_Window* window);
   void InitRender(ID3D12Device* device, ID3D12DescriptorHeap* heap,
                   std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> handles);
   void OnUpdateRender();
   void Render(ID3D12GraphicsCommandList* commandList);
};
