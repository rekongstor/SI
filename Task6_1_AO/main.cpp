#include "../Task5_1_DirectX12/Window.h"
#include "RendererAO.h"

int main()
{
   Window window(L"Cacao", 512, 512);

   window.OnInit();


   if (window.getWindow())
   {
      RendererAO renderer(&window);
      renderer.onInit();

      MSG msg;
      while (true)
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
