#include "core_Window.h"
#include <imgui.h>

LRESULT imguiProcDummy(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   return false;
}

LRESULT(*imguiProc)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) = imguiProcDummy;

LRESULT winProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   if (imguiProc(hWnd, msg, wParam, lParam))
      return 0;

   switch (msg) {
   case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
   default:
      return DefWindowProc(hWnd, msg, wParam, lParam);
   }
}

bool core_Window::OnInit(int width, int height, LONG winStyle)
{
   assert(width > 0 && height > 0);
   this->width = width;
   this->height = height;
   this->winStyle = winStyle;

   wndClass = {
   sizeof(WNDCLASSEX), CS_DBLCLKS, winProc,
   0, 0, GetModuleHandle(nullptr), LoadIcon(nullptr,IDI_APPLICATION),
   LoadCursor(nullptr,IDC_ARROW), HBRUSH(COLOR_MENU),
   nullptr, name, LoadIcon(nullptr,IDI_APPLICATION)
   };
   this->winStyle = winStyle;

   if (RegisterClassEx(&wndClass)) {
      window = CreateWindowEx(0, name, name, winStyle, 0, 0,
         width, height, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
      if (window) {
         if (window) {
            ShowWindow(window, SW_SHOWDEFAULT);
            UpdateWindow(window);
         }
      }
      return true;
   }
   return false;
}

void core_Window::SetWindowZorderToTopMost(bool setToTopMost)
{

   RECT windowRect;
   GetWindowRect(window, &windowRect);

   SetWindowPos(
      window,
      (setToTopMost) ? HWND_TOPMOST : HWND_NOTOPMOST,
      windowRect.left,
      windowRect.top,
      windowRect.right - windowRect.left,
      windowRect.bottom - windowRect.top,
      SWP_FRAMECHANGED | SWP_NOACTIVATE);
}
