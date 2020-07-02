#pragma once
#include "Windows.h"
class siImgui;

class siWindow
{
   friend class siImgui;

   const WCHAR* name;
   int width;
   int height;
   WNDCLASSEX wndClass = {};
   HWND window = nullptr;

   static LRESULT winProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

   static LRESULT imguiProcDummy(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
   static LRESULT (*imguiProc)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
public:
   explicit siWindow(const WCHAR* name, int width, int height);

   [[nodiscard]] HWND getWindow() const;
   [[nodiscard]] uint32_t getWidth() const;
   [[nodiscard]] uint32_t getHeight() const;
   void onInit();
};

