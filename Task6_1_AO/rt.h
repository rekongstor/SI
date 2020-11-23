#ifndef RT_H
#define RT_H

struct Viewport
{
   float left;
   float top;
   float right;
   float bottom;
};

struct RayGenConstantBuffer
{
   Viewport viewport;
   Viewport stencil;
};

#endif // RT_H
#define RT_H