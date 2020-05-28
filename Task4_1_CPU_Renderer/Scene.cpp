#define _USE_MATH_DEFINES
#include "Scene.h"
#include "../Core/Bitmap.h"

Scene::Scene(const Camera& camera): camera(camera)
{
}

void Scene::addObject(Object* object)
{
   objects.push_back(object);
}

void Scene::renderScene(uint32_t width, uint32_t height, const char* filename)
{
   std::vector<Ray> rays;
   rays.resize(width * height);

   // Calculating basis
   Point3D up = {0.f, 0.f, 1.f};
   Point3D u = normalize(mul(camera.direction, up));
   Point3D w = mul(u, camera.direction);

   // Calculating plane
   Point3D left = rotate(camera.direction, w, camera.fov * M_PI * 0.125f);
   Point3D right = rotate(camera.direction, w, -camera.fov * M_PI * 0.125f);
   float side = -sqrtf(2.f / (1.f - cosf(camera.fov * M_PI * 0.5f)));
   left = left * side;
   right = right * side;
   float planeDistance = length(mul(left, right)); // half area for triangle and another half for base (2.0)
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
         bitmap.SetPixel(i, j);
      }
}
