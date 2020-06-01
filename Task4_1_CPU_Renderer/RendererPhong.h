#pragma once
#include "Renderer.h"

class RendererPhong :
   public Renderer
{
   Color pixelShader(constantBuffer buffer, Light light, const Ray& ray) override;
public:
   RendererPhong(const Camera& camera, const Color& ambientColor, const Color& voidColor);
   ~RendererPhong() = default;
};
