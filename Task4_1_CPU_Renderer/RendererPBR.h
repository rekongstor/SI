#pragma once
#include "Renderer.h"

class RendererPBR :
   public Renderer
{
   Color pixelShader(std::tuple<Color, Color, float, float, float, Point3D, float> buffer, Light light, const Ray& ray) override;
public:
   RendererPBR(const Camera& camera, const Color& ambientColor);
   ~RendererPBR() = default;
};
