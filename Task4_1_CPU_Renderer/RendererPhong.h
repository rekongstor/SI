#pragma once
#include "Renderer.h"

class RendererPhong :
   public Renderer
{
   Color pixelShader(std::tuple<Color, Color, float, float, float, Point3D, float> buffer, Light light, const Ray& ray) override;
public:
   RendererPhong(const Camera& camera, const Color& ambientColor);
   ~RendererPhong() = default;
};
