#define _USE_MATH_DEFINES
#include "Renderer.h"
#include "Scene.h"
#include "../Core/Bitmap.h"


Renderer::Renderer(const Camera& camera, const Color& ambientColor) : camera(camera),
                                                                      ambientColor(ambientColor)
{
}


std::tuple<Color, Color, float, Point3D> Renderer::rayCast(Scene& scene, Ray ray)
{
   // Finding closest pixel color
   std::pair<Point3D, float> closest;
   closest.second = std::numeric_limits<float>::infinity();
   std::tuple<Color, Color, float> color;
   for (auto& obj : scene.objects)
   {
      if (obj->anyHit(ray))
      {
         auto hit = obj->closestHit(ray);
         if (hit.second < closest.second)
         {
            closest = hit;
            color = obj->getColor();
         }
      }
   }
   auto [diffuseColor, specularColor, specularExp] = color;
   // Make shadow
   Ray secondRay = { ray.origin + ray.direction * (closest.second - 0.001f),scene.light.direction };
   Color black = { 0.f,0.f,0.f };
   for (auto& obj : scene.objects)
   {
      if (obj->anyHit(secondRay))
         return { black, black, 1.f,closest.first };
         
   }
   return {diffuseColor, specularColor, specularExp, closest.first};
}

void Renderer::renderScene(Scene& scene, uint32_t width, uint32_t height, const char* filename)
{
   camera.position.z = -camera.position.z;
   std::vector<Ray> rays;
   rays.resize(width * height);

   // Calculating basis
   Point3D up = {0.f, 0.f, 1.f};
   Point3D u = mul(camera.direction, up);
   if (length(up) > 0.001f)
      u = normalize(u);
   else
      u = {1.f, 0.f, 0.f};
   Point3D w = mul(u, camera.direction);

   // Calculating plane
   Point3D left = rotate(camera.direction, w, camera.fov * M_PI * 0.125f);
   Point3D right = rotate(camera.direction, w, -camera.fov * M_PI * 0.125f);
   float side = -sqrtf(2.f / (1.f - cosf(camera.fov * M_PI * 0.5f)));
   left = left * side;
   right = right * side;
   float planeDistance = length(mul(left, right)) * 0.25;
   Point3D uwPosition = camera.position + (camera.direction * planeDistance);
   uwPosition = uwPosition - u - w; // moving to (-1;-1) uw coordinate

   Point3D deltaX = u * 2.f / static_cast<float>(width);
   Point3D deltaY = w * 2.f / static_cast<float>(height);

   // Creating rays
   for (auto i = 0; i < width; ++i)
   {
      for (auto j = 0; j < height; ++j)
      {
         rays[i * width + j] = {{uwPosition}, {normalize(uwPosition - camera.position)}};
         uwPosition = uwPosition + deltaX;
      }
      uwPosition = uwPosition - u - u + deltaY;
   }

   // Calculating rays
   Bitmap bitmap(width, height);
   for (auto i = 0; i < width; ++i)
      for (auto j = 0; j < height; ++j)
      {
         Color color = pixelShader(rayCast(scene, rays[i * width + j]), scene.light, rays[i * width + j]);
         bitmap.SetPixel(j, i, {
                            static_cast<uint8_t>(color.r * 255.f), static_cast<uint8_t>(color.g * 255.f),
                            static_cast<uint8_t>(color.b * 255.f)
                         });
      }
   bitmap.Save(filename);
}
