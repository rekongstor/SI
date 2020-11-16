#include <corecrt_math_defines.h>

#include "siWindow.h"
#include "siImgui.h"
#include "siRenderer.h"

void GenerateSSAONoise()
{
   int sx = 3;
   int sy = 3;

   float4 pix;
   pix.z = pix.w = 0;
   for (int y = 0; y < sy; y++) {
      for (int x = 0; x < sx; x++) {
         float angle = 2 * M_PI * (y * sx + x) / (sx * sy);
         pix.x = cosf(angle) * 0.5f + 0.5f; pix.y = sinf(angle) * 0.5f + 0.5f;
         pix.z = -sinf(angle) * 0.5f + 0.5f; pix.w = cosf(angle) * 0.5f + 0.5f;
         std::cout << pix.x << ", " << pix.y << ", " << pix.z << ", " << pix.w << ", " << std::endl;
      }
   }
}
int main()
{
   GenerateSSAONoise();
   siImgui imgui;
   imgui.onInit();

   siWindow window(L"Cacao", 640, 480);
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
