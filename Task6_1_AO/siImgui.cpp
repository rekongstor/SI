#include "siImgui.h"
#include "siWindow.h"
#include <imgui.h>
#include <examples/imgui_impl_dx12.h>
#include <examples/imgui_impl_win32.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

siImgui::siImgui()
{
   siWindow::imguiProc = ImGui_ImplWin32_WndProcHandler;
}

void siImgui::onInit()
{
   std::cout << "Initializing ImGUI context" << std::endl;

   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   ImGuiIO& io = ImGui::GetIO(); (void)io;

   ImGui::StyleColorsDark();
}

void siImgui::onInitWindow(HWND window)
{
   std::cout << "Initializing ImGUI window" << std::endl;

   ImGui_ImplWin32_Init(window);
}

void siImgui::onInitRenderer(ID3D12Device* device, uint32_t bufferCount, ID3D12DescriptorHeap* heap, std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE> handles)
{
   std::cout << "Initializing ImGUI renderer" << std::endl;

   ImGui_ImplDX12_Init(device, bufferCount,
      DXGI_FORMAT_R8G8B8A8_UNORM, heap,
      handles.first,
      handles.second);
}

void siImgui::onUpdate()
{
   ImGui_ImplDX12_NewFrame();
   ImGui_ImplWin32_NewFrame();
   ImGui::NewFrame();
   {
      ImGui::Begin("Imgui Debug");
      ImGui::End();
   }
}

void siImgui::onRender(ID3D12GraphicsCommandList* commandList)
{
   ImGui::Render();
   ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}
