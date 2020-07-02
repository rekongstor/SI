#include "siWindow.h"
#include "siImgui.h"
#include "siRenderer.h"

int main()
{
   siWindow window(L"Cacao", 512, 512);
   window.onInit();
   
   if (window.getWindow())
   {
      siImgui imgui(&window);
      imgui.onInit();
      siRenderer renderer(&window, 3);
      renderer.onInit();

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

      renderer.onDestroy();
   }
   return 0;
}
