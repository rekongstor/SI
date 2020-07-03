#include "Window.h"
#include "stdafx.h"
#include <examples/imgui_impl_win32.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

WinProc winProc(HWND window, unsigned msg, WPARAM wp, LPARAM lp)
{
   if (ImGui_ImplWin32_WndProcHandler(window, msg, wp, lp))
      return true;

   switch (msg)
   {
   case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
   default:
      return DefWindowProc(window, msg, wp, lp);
   }
}


int Window::getWidth() const
{
   return width;
}

int Window::getHeight() const
{
   return height;
}

HWND Window::getWindow() const
{
   return window;
}

void Window::OnInit()
{
   if (RegisterClassEx(&wndClass))
   {
      window = CreateWindowEx(0, name, name, WS_POPUPWINDOW, 0, 0,
                                   width, height, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
      if (window)
      {
         ShowWindow(window, SW_SHOWDEFAULT);
         UpdateWindow(window);
      }
   }
}

Window::Window(const WCHAR* name, int width, int height): name(name), width(width), height(height)
{
   wndClass = {
      sizeof(WNDCLASSEX), CS_DBLCLKS, winProc,
      0, 0, GetModuleHandle(nullptr), LoadIcon(nullptr,IDI_APPLICATION),
      LoadCursor(nullptr,IDC_ARROW), HBRUSH(COLOR_WINDOW + 2),
      nullptr, name,LoadIcon(nullptr,IDI_APPLICATION)
   };
}
