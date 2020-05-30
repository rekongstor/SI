#include "RendererPBR.h"

#include <corecrt_math_defines.h>


RendererPBR::RendererPBR(const Camera& camera, const Color& ambientColor): Renderer(camera, ambientColor)
{
}

Color RendererPBR::pixelShader(std::tuple<Color, Color, float, float, float, Point3D> buffer, Light light,
                               const Ray& ray)
{
   // Implementing phong
   auto& [diffuseColor, specularColor, specularExp, metalness, roughness, normal] = buffer;
   if (length(normal) < 0.9f)
      return {0.5f, 0.5f, 0.5f};
   float k = dot(normal, light.direction);
   if (k > 0.f)
   {
      roughness = roughness * 0.35f + 0.01f; // ensure roughness is not 0
      const Point3D& L = light.direction;
      const Point3D& N = normal;
      const Point3D& V = ray.direction * -1.f;
      const Point3D H = normalize(L + V);
      float dotLH = dot(L, H);
      float dotNH = dot(N, H);
      float dotNV = dot(N, V);
      float dotNL = dot(N, L);

      auto getRefractIndex = [](const float specularValue)
      {
         return (1.f + sqrtf(specularValue)) / (1.f - sqrtf(specularValue));
      };
      auto getG = [&dotLH](const float refractValue)
      {
         return sqrtf(refractValue * refractValue - 1.f + dotLH * dotLH);;
      };
      auto getFresnelValue = [&dotLH](const float gValue)
      {
         return 0.5f *
            ((gValue - dotLH) * (gValue - dotLH)) / ((gValue + dotLH) * (gValue + dotLH)) *
            ((dotLH * (gValue + dotLH) - 1.f) * (dotLH * (gValue + dotLH) - 1.f) / (dotLH * (gValue - dotLH) + 1.f) * (
               dotLH * (gValue - dotLH) + 1.f) + 1.f);
      };

      float refractRed = getRefractIndex(specularColor.r);
      float refractGreen = getRefractIndex(specularColor.g);
      float refractBlue = getRefractIndex(specularColor.b);
      float gRed = getG(refractRed);
      float gGreen = getG(refractGreen);
      float gBlue = getG(refractBlue);
      Color fresnelFactor = {getFresnelValue(gRed), getFresnelValue(gGreen), getFresnelValue(gBlue)};

      float isotropicMicroFacet = 1.f / (4.f * roughness * roughness * dotNH * dotNH * dotNH * dotNH) *
         expf((dotNH * dotNH - 1.f) / (roughness * roughness * dotNH * dotNH));
      float geometricalAttenuationFactor = std::min(
         {
            1.f,
            2.f * dotNH * dotNV / dotLH,
            2.f * dotNH * dotNL / dotLH
         }
      );

      Color specularPBR = fresnelFactor * isotropicMicroFacet * geometricalAttenuationFactor /
         (M_PI * dotNV * dotNL);

      Color color = ambientColor +
         diffuseColor * (1.f - metalness) * light.color * k +
         specularPBR * (metalness) * light.color;
      //return { abs(normal.x), abs(normal.y), abs(normal.z) };
      return color;
   }
   else
      return ambientColor;
}
