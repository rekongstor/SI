#include "siImgui.h"
#include "siWindow.h"
#include <imgui.h>
#include <examples/imgui_impl_win32.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

siImgui::siImgui(siWindow* window): window(window)
{
   siWindow::imguiProc = ImGui_ImplWin32_WndProcHandler;
}

void siImgui::onInit() const
{
   if (!window)
      return;
   // Setup Dear ImGui context
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   ImGuiIO& io = ImGui::GetIO(); (void)io;
   //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
   //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

   // Setup Dear ImGui style
   ImGui::StyleColorsDark();
   //ImGui::StyleColorsClassic();

   // Setup Platform/Renderer bindings
   ImGui_ImplWin32_Init(window);
}
