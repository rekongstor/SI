#pragma once
class siWindow;

class siImgui
{
   siWindow* window = nullptr;
public:
   explicit siImgui(siWindow* window);

   void onInit() const;
};
