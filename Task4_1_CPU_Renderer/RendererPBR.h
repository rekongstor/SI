#pragma once
#include "Renderer.h"

class RendererPBR :
   public Renderer
{
   Color pixelShader(std::tuple<Color, Color, float, Point3D> buffer, Light light, const Ray& ray) override;
public:
   RendererPBR(const Camera& camera, const Color& ambientColor);
   ~RendererPBR() = default;
};
