#include "RendererPBR.h"

#include <corecrt_math_defines.h>
#define GAMMA 2.2f

RendererPBR::RendererPBR(const Camera& camera, const Color& ambientColor, const Color& voidColor):
   Renderer(camera, ambientColor, voidColor)
{
}

Color RendererPBR::pixelShader(constantBuffer buffer, Light light, const Ray& ray)
{
   // Implementing phong
   auto& [diffuseColor, specularColor, specularExp, metalness, roughness, normal, dist] = buffer;
   if (length2(normal) < 0.9f)
      return voidColor;
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
      Color normDiffuseColor = {powf(diffuseColor.r, GAMMA), powf(diffuseColor.g, GAMMA), powf(diffuseColor.b, GAMMA)};

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
         return geomGGX(dotNV, rough) * geomGGX(dotNL, rough);
      };

      // Normalizing given metalness
      float normMetalness = std::clamp(metalness, 0.f, 1.f) * 0.96f + 0.04f;
      Color F = {
         fresnelValue(normMetalness * normDiffuseColor.r), fresnelValue(normMetalness * normDiffuseColor.g),
         fresnelValue(normMetalness * normDiffuseColor.b)
      };

      // Normalizing given roughness
      float normRoughness = std::clamp(roughness, 0.f, 1.f) * 0.95f + 0.05f;
      float NDF = distGGX(normRoughness);
      float G = geomValue(normRoughness);

      Color specular = F * NDF * G / std::max(4.f * dotNV * dotNL, 0.001f);
      Color kS = F * -1.f;
      Color kD = (kS + 1.f) * (1.f - normMetalness);
      Color color = ambientColor * normDiffuseColor +
         (normDiffuseColor * kD / M_PI + specular) * light.color * dotNL;;

      color = color / (color + 1.f);
      color = {powf(color.r, 1.f / GAMMA), powf(color.g, 1.f / GAMMA), powf(color.b, 1.f / GAMMA)};
      return color;
   }
   return ambientColor;
}
