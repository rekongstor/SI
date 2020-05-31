#include "RendererPhong.h"


RendererPhong::RendererPhong(const Camera& camera, const Color& ambientColor): Renderer(camera, ambientColor)
{
}

Color RendererPhong::pixelShader(constantBuffer buffer, Light light, const Ray& ray)
{
   // Implementing phong
   auto& [diffuseColor, specularColor, specularExp, metalness, roughness, normal, dist] = buffer;
   if (length(normal) < 0.9f)
      return {0.5f, 0.5f, 0.5f};
   float k = dot(normal, light.direction);
   if (k > 0.f)
   {
      float spec = dot(normal, normalize(light.direction - ray.direction));
      Color color = ambientColor +
         diffuseColor * light.color * k +
         specularColor * light.color * powf(std::max(spec, 0.f), specularExp);
      return color;
   }
   return ambientColor;
}
