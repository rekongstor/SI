#pragma once
#include <tuple>

#include "Structures.h"

class Scene;

class Renderer
{
   std::tuple<Color, Color, float, float, float, Point3D> rayCast(Scene& scene, Ray ray);
protected:
   Camera camera;
   Color ambientColor;
   virtual Color pixelShader(std::tuple<Color, Color, float, float, float, Point3D> buffer, Light light,
                             const Ray& ray) = 0;
public:
   void renderScene(Scene& scene, uint32_t width, uint32_t height, const char* filename, float gamma, float exposure);


   Renderer(const Camera& camera, const Color& ambientColor);

   virtual ~Renderer() = default;
};
