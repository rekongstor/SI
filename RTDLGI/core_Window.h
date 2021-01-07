#pragma once

class core_Window
{
public:
   bool OnInit(int width, int height, LONG winStyle);
   bool IsFullscreen() { return fullscreen; }
   void SetWindowZorderToTopMost(bool setToTopMost);
   

   const WCHAR* name = L"RTDLGI";
   UINT width;
   UINT height;
   WNDCLASSEX wndClass = {};
   HWND window = nullptr;
   LONG winStyle;
   bool fullscreen = false;
};

