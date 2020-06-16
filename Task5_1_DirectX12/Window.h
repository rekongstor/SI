#pragma once
#include <Windows.h>

class Window
{
   const WCHAR* name;
   int width;
   int height;
   WNDCLASSEX wndClass;
   HWND window;
public:
   [[nodiscard]] HWND getWindow() const;
   [[nodiscard]] int getWidth() const;
   [[nodiscard]] int getHeight() const;
   void OnInit();

   explicit Window(const WCHAR* name, int width, int height);
};

