#include "Dx12Renderer.h"
#include "Window.h"
#include "stdafx.h"
#include <iostream>


int main()
{
   // This should depend on localization
   setlocale(LC_ALL, "Russian");


   Window window(L"SI", 1280, 720);
   window.OnInit();
#ifdef _DEBUG
   try {
#endif
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
#ifdef _DEBUG
   }
   catch (std::exception& e)
   {
      std::cout << e.what() << std::endl;
   }
#endif
   return 0;
}
