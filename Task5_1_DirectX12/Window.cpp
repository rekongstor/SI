#include "Window.h"
#include "stdafx.h"

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
