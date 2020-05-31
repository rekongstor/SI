#include "RendererPBR.h"

#include <corecrt_math_defines.h>


RendererPBR::RendererPBR(const Camera& camera, const Color& ambientColor): Renderer(camera, ambientColor)
{
}

Color RendererPBR::pixelShader(constantBuffer buffer, Light light, const Ray& ray)
{
   // Implementing phong
   auto& [diffuseColor, specularColor, specularExp, metalness, roughness, normal, dist] = buffer;
   if (length(normal) < 0.9f)
      return {0.8f, 1.0f, 1.0f};
   float k = dot(normal, light.direction);
   if (k > 0.f)
   {
      const Point3D& L = light.direction;
      const Point3D& N = normal;
      const Point3D& V = ray.direction * -1.f;
      const Point3D H = normalize(L + V);
      float dotLH = std::max(dot(L, H), 0.f);
      float dotNH = std::max(dot(N, H), 0.f);
      float dotNV = std::max(dot(N, V), 0.f);
      float dotNL = std::max(dot(N, L), 0.f);

      // Calculating normalized diffuse 
      Color normDiffuseColor = {powf(diffuseColor.r, 2.2f), powf(diffuseColor.g, 2.2f), powf(diffuseColor.b, 2.2f)};

      auto fresnelValue = [&dotLH](float metal) -> float
      {
         return metal + (1.f - metal) * powf(1.f - dotLH, 5.f);
      };
      auto distGGX = [&dotNH](float rough) -> float
      {
         float a4 = rough * rough * rough * rough;
         float denom = dotNH * dotNH * (a4 - 1.f) + 1.f;

         return a4 / (M_PI * denom * denom);
      };
      auto geomGGX = [](float dotNV, float rough) -> float
      {
         float r = rough + 1.f;
         float k = r * r / 8.f;
         return dotNV / (dotNV * (1.f - k) + k);
      };
      auto geomValue = [&geomGGX, &dotNV, &dotNL](float rough) -> float
      {
         return geomGGX(rough, dotNV) * geomGGX(dotNL, rough);
      };

      float normMetalness = metalness * 0.9f + 0.1f;
      Color F = {
         fresnelValue(normMetalness * normDiffuseColor.r), fresnelValue(normMetalness * normDiffuseColor.g),
         fresnelValue(normMetalness * normDiffuseColor.b)
      };

      float normRoughness = roughness * 0.95f + 0.05f;
      float NDF = distGGX(normRoughness);
      float G = geomValue(normRoughness);

      Color specular = F * NDF * G / std::max(4.f * dotNV * dot(N, L), 0.001f);
      Color kS = F * -1.f;
      Color kD = (kS + 1.f) * (1.f - normMetalness);
      Color color = ambientColor * normDiffuseColor +
         (normDiffuseColor * kD / M_PI + specular) * light.color * dotNL;;

      color = color / (color + 1.f);
      color = {powf(color.r, 1.f / 2.2f), powf(color.g, 1.f / 2.2f), powf(color.b, 1.f / 2.2f)};
      return color;
   }
   return ambientColor;
}
