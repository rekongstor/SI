#include "Dx12Renderer.h"
#include "Window.h"
#include "stdafx.h"
#include <iostream>
#include "../3rd_party/imgui/misc/single_file/imgui_single_file.h"
#include "../3rd_party/imgui/examples/imgui_impl_win32.h"

int main()
{
   Window window(L"SI", 1024, 768);
   window.OnInit();
      if (window.getWindow())
      {
         IMGUI_CHECKVERSION();
         ImGui::CreateContext();
         ImGuiIO& io = ImGui::GetIO(); (void)io;
         //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
         //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

         // Setup Dear ImGui style
         ImGui::StyleColorsDark();
         //ImGui::StyleColorsClassic();

         // Setup Platform/Renderer bindings
         ImGui_ImplWin32_Init(window.getWindow());
         Dx12Renderer renderer(&window, 3);
         renderer.OnInit();

         MSG msg;
         while (renderer.isActive())
         {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
               TranslateMessage(&msg);
               DispatchMessage(&msg);
            }
            else
            {
               renderer.OnUpdate();
            }
         }

         renderer.OnDestroy();
      }
   return 0;
}
