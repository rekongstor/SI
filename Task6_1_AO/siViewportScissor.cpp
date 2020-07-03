#include "siViewportScissor.h"

siViewportScissor::siViewportScissor(uint32_t width, uint32_t height) :
   viewport(0.f, 0.f, static_cast<float>(width), static_cast<float>(height), 0.f, 1.f),
   scissorRect(0, 0, width, height)
{
}

const CD3DX12_VIEWPORT& siViewportScissor::getViewport() const
{
   return viewport;
}

const CD3DX12_RECT& siViewportScissor::getScissorRect() const
{
   return scissorRect;
}
