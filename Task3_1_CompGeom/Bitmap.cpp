#include "Bitmap.h"

Bitmap::Bitmap(uint32_t width, uint32_t height) : bitmapImage(bitmap_image(width, height))
{
   bitmapImage.clear(255);
}

void Bitmap::DrawLine(float x0, float y0, float x1, float y1, int red, int green, int blue)
{
   image_drawer draw(bitmapImage);
   draw.pen_width(1);
   draw.pen_color(red, green, blue);
   draw.line_segment(x0, bitmapImage.height() -y0, x1, bitmapImage.height() -y1);
}

void Bitmap::DrawPoint(float x, float y)
{
   image_drawer draw(bitmapImage);
   draw.pen_width(1);
   draw.pen_color(255, 0, 0);
   draw.circle(x, bitmapImage.height()-y, 1);
}

void Bitmap::Save()
{
   bitmapImage.save_image("triangulation.bmp");
}
