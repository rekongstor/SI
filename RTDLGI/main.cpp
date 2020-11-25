#include "core_Window.h"
#include "core_Imgui.h"
#include "rnd_DX12.h"

bool procRunning = true;

int main()
{
   core_Imgui imgui;
   imgui.OnInit();
   core_Window window;
   if (!window.OnInit(640, 480, WS_POPUPWINDOW))
      return 1;

   rnd_Dx12 renderer;
   renderer.OnInit(&window);

   imgui.InitWindow(&window);
   MSG msg;
   while (procRunning)
   {
      if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      } else {
         renderer.OnUpdate();
      }
   }

   return 0;
}