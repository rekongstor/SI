#pragma once
#include <tuple>

#include "Structures.h"

class Scene;

struct constantBuffer
{
   const Color diffuseColor;
   const Color specularColor;
   const float specularExp;
   const float metalness;
   const float roughness;
   const Point3D normal;
   const float distance;
};

class Renderer
{
   static constantBuffer rayCast(Scene& scene, Ray ray);
protected:
   Camera camera;
   Color ambientColor;
   Color voidColor;
   virtual Color pixelShader(constantBuffer buffer, Light light, const Ray& ray) = 0;
public:
   void renderScene(Scene& scene, uint32_t width, uint32_t height, const char* filename);


   Renderer(const Camera& camera, const Color& ambientColor, const Color& voidColor);

   virtual ~Renderer() = default;
};
