#pragma once
#include "Renderer.h"

class RendererPBR :
   public Renderer
{
   Color pixelShader(constantBuffer buffer, Light light, const Ray& ray) override;
public:
   RendererPBR(const Camera& camera, const Color& ambientColor, const Color& voidColor);
   ~RendererPBR() = default;
};
