#include "Dx12Renderer.h"
#include "Window.h"
#include "stdafx.h"
#include <iostream>


int main()
{
   Window window(L"SI", 1024, 1024);
   window.OnInit();
      if (window.getWindow())
      {
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
