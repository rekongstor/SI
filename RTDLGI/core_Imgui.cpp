#include "core_Imgui.h"
#include "core_Window.h"
#include "rnd_Dx12.h"
#include <imgui.h>
#include <examples/imgui_impl_dx12.h>
#include <examples/imgui_impl_win32.h>

#include "SceneConstBuf.h"

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
   imguiProc = ImGui_ImplWin32_WndProcHandler;
}

void core_Imgui::InitWindow()
{
   assert(window != NULL && window->window != NULL);

   ImGui_ImplWin32_Init(window->window);
}

void core_Imgui::InitRender()
{
   auto handle = renderer->GetCbvSrvUavHandle();
   ImGui_ImplDX12_Init(renderer->device.Get(), FRAME_COUNT,
                       renderer->swapChainFormat, renderer->cbvSrvUavHeap.Get(),
                       handle.first,
                       handle.second);
}

void core_Imgui::OnUpdate()
{
   ImGui_ImplDX12_NewFrame();
   ImGui_ImplWin32_NewFrame();
   ImGui::NewFrame();

#pragma region "Debug window"
   ImGui::Begin("Camera");
   ImGui::DragFloat3("Position", &renderer->camPos.x, 0.01);
   ImGui::DragFloat2("Pitch/Yaw", &renderer->camDir.x, 0.01);
   ImGui::DragFloat("Fov", &renderer->fovAngleY, 0.1);
   ImGui::End();
#pragma endregion
}

void core_Imgui::OnRender()
{
   ImGui::Render();
   ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), renderer->CommandList());
}
