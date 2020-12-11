#include "core_Imgui.h"
#include "core_Window.h"
#include <imgui.h>
#include <examples/imgui_impl_dx12.h>
#include <examples/imgui_impl_win32.h>

extern bool procRunning;
extern LRESULT (*imguiProc)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void core_Imgui::OnInit()
{
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   ImGuiIO& io = ImGui::GetIO();
   (void)io;
   io.AddInputCharacter('w');
   io.AddInputCharacter('a');
   io.AddInputCharacter('s');
   io.AddInputCharacter('d');
   io.KeyRepeatDelay = 0.001f;
   io.KeyRepeatRate = 0.001f;
   io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

   ImGui::StyleColorsDark();
}

void core_Imgui::InitWindow(core_Window* window)
{
   assert(window != NULL && window->window != NULL);

   ImGui_ImplWin32_Init(window->window);
   imguiProc = ImGui_ImplWin32_WndProcHandler;
}

void core_Imgui::InitRender(ID3D12Device* device, ID3D12DescriptorHeap* heap,
                           std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> handles)
{
   assert(device != NULL);
   assert(heap != NULL);
   assert(handles.first.ptr != NULL);
   assert(handles.second.ptr != NULL);

   ImGui_ImplDX12_Init(device, FRAME_COUNT,
                       DXGI_FORMAT_R8G8B8A8_UNORM, heap,
                       handles.first,
                       handles.second);
}

void core_Imgui::OnUpdateRender()
{
   ImGui_ImplDX12_NewFrame();
   ImGui_ImplWin32_NewFrame();
   ImGui::NewFrame();

#pragma region "Debug window"
   ImGui::Begin("Imgui Debug");

   if (ImGui::Button("Stop"))
   {
      procRunning = true;
   }

   ImGui::Text("Sas");

   ImGui::End();
#pragma endregion
}

void core_Imgui::Render(ID3D12GraphicsCommandList* commandList)
{
   assert(commandList != NULL);

   ImGui::Render();
   ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}
