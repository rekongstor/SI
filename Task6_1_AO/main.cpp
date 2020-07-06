#include "siWindow.h"
#include "siImgui.h"
#include "siRenderer.h"

int main()
{
   siImgui imgui;
   imgui.onInit();

   siWindow window(L"Cacao", 512, 512);
   window.onInit();
   
   if (window.getWindow())
   {
      imgui.onInitWindow(window.getWindow());
      siRenderer renderer(&window, 3);
      renderer.onInit(&imgui);

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
            renderer.onUpdate();
         }
      }
   }
   return 0;
}
