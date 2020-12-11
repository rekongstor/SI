#include "core_Window.h"
#include "core_Imgui.h"
#include "rnd_DX12.h"

bool procRunning = true;
rnd_Dx12* renderer;

int main()
{
   core_Imgui imgui;
   core_Window window;
   rnd_Dx12 rendererInst;
   renderer = &rendererInst;

   imgui.OnInit();
   if (!window.OnInit(640, 480, WS_POPUPWINDOW))
      return 1;

   renderer->OnInit(&window);

   imgui.InitWindow(&window);
   //imgui.InitRender(renderer->device.Get(), renderer->dsvHeap.Get(), renderer->GetCbvSrvUavHandle());
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