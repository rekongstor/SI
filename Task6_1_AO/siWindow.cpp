#include "siWindow.h"
#include <imgui.h>

siWindow::siWindow(const WCHAR* name, int width, int height) : name(name), width(width), height(height)
{
   wndClass = {
      sizeof(WNDCLASSEX), CS_DBLCLKS, winProc,
      0, 0, GetModuleHandle(nullptr), LoadIcon(nullptr,IDI_APPLICATION),
      LoadCursor(nullptr,IDC_ARROW), HBRUSH(COLOR_WINDOW),
      nullptr, name,LoadIcon(nullptr,IDI_APPLICATION)
   };
}

LRESULT siWindow::winProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   if (imguiProc(hWnd, msg, wParam, lParam))
      return true;

   switch (msg)
   {
   case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
   default:
      return DefWindowProc(hWnd, msg, wParam, lParam);
   }
}

void siWindow::onInit()
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

uint32_t siWindow::getWidth() const
{
   return width;
}

uint32_t siWindow::getHeight() const
{
   return height;
}

HWND siWindow::getWindow() const
{
   return window;
}

LRESULT siWindow::imguiProcDummy(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   return false;
}

LRESULT (*siWindow::imguiProc)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) = imguiProcDummy;
