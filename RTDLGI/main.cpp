#include "core_Window.h"
#include "core_Imgui.h"
#include "rnd_DX12.h"

bool procRunning = true;
core_Imgui* imgui = nullptr;
core_Window* window = nullptr;
rnd_Dx12* renderer = nullptr;

extern LRESULT(*imguiProc)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int main()
{
   core_Imgui imguiInst;
   core_Window windowInst;
   rnd_Dx12 rendererInst;

   //imgui = &imguiInst;
   window = &windowInst;
   renderer = &rendererInst;

   imguiInst.OnInit();

   if (!window->OnInit(640, 480, WS_POPUPWINDOW))
      return 1;

   imguiInst.InitWindow();
   renderer->OnInit();

   MSG msg;
   while (procRunning)
   {
      if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      } else {
         renderer->OnUpdate();
      }
   }

   return 0;
}