#include "Dx12Renderer.h"
#include "Window.h"
#include "stdafx.h"
#include <iostream>

WinProc winProc(HWND window, unsigned msg, WPARAM wp, LPARAM lp)
{
   switch (msg)
   {
   case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
   default:
      return DefWindowProc(window, msg, wp, lp);
   }
}

int main()
{
   Window window(L"SI", 640, 480);
   window.OnInit();
#ifdef _DEBUG
   try {
#endif
      if (window.getWindow())
      {
         Dx12Renderer renderer(&window, 2);
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
   catch (std::exception e)
   {
      std::cout << e.what() << std::endl;
   }
#endif
   return 0;
}
