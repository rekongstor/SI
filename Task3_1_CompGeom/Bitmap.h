#pragma once
#include <list>

#include "../3rd_party/bitmap/bitmap_image.hpp"

class Bitmap
{
   bitmap_image bitmapImage;
public:
   Bitmap(uint32_t width, uint32_t height);
   void DrawLine(float x0, float y0, float x1, float y1, int red = 0, int green = 0, int blue = 0);
   void DrawPoint(float x, float y);
   void Save();
};
