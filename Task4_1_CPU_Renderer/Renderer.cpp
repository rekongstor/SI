#define _USE_MATH_DEFINES
#include "Renderer.h"
#include "Scene.h"
#include "../Core/Bitmap.h"


Renderer::Renderer(const Camera& camera, const Color& ambientColor) : camera(camera),
                                                                      ambientColor(ambientColor)
{
}


std::tuple<Color, Color, float, float, float, Point3D, float> Renderer::rayCast(Scene& scene, Ray ray)
{
   // Finding closest pixel color
   std::pair<Point3D, float> closest;
   closest.second = std::numeric_limits<float>::infinity();
   Color diffuseColor, specularColor;
   float specularExp, metalness, roughness;
   for (auto& obj : scene.objects)
   {
      if (obj->anyHit(ray))
      {
         auto hit = obj->closestHit(ray);
         if (hit.second < closest.second)
         {
            closest = hit;
            diffuseColor = obj->getDiffuseColor();
            specularColor = obj->getSpecularColor();
            specularExp = obj->getSpecularExp();
            metalness = obj->getMetalness();
            roughness = obj->getRoughness();
         }
      }
   }
   // Make shadow
   Ray secondRay = {ray.origin + ray.direction * (closest.second), scene.light.direction};
   Color black = {0.f, 0.f, 0.f};
   for (auto& obj : scene.objects)
   {
      if (obj->anyHit(secondRay))
         return {black, black, 1.f, 0.f, 0.f, closest.first, closest.second};
   }
   return {diffuseColor, specularColor, specularExp, metalness, roughness, closest.first, closest.second };
}

void Renderer::renderScene(Scene& scene, uint32_t width, uint32_t height, const char* filename, float gamma,
                           float exposure)
{
   camera.position.z = -camera.position.z;
   std::vector<Ray> rays;
   rays.resize(width * height);

   // Calculating basis
   Point3D up = {0.f, 0.f, 1.f};
   Point3D u;
   if (length(up - camera.direction) > 0.01f)
   {
      u = normalize(mul(camera.direction, up));
   }
   else
   {
      u = {1.f, 0.f, 0.f};
   }
   Point3D w = mul(u, camera.direction);

   // Calculating plane
   Point3D left = rotate(camera.direction, w, camera.fov * M_PI / 180.f);
   Point3D right = rotate(camera.direction, w, -camera.fov * M_PI / 180.f);
   float side = -sqrtf(2.f / (1.f - cosf(camera.fov * M_PI / 180.f)));
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
         // First ray
         Ray ray = rays[i * width + j];
         auto [diff, spec, spExp, met, rough, normal, distance] = rayCast(scene, ray);
         Color firstColor = pixelShader({ diff, spec, spExp, met, rough, normal, distance }, scene.light, ray);
         Color secondColor;
         if (length(normal) > 0.9f)
         {
            Point3D L = ray.direction * -1.f;
            Point3D R = normal * 2.f * dot(normal, L) - L;
            //mix = std::max(0.f,-dot(normal, L));
            // Second ray
            Ray secondRay = { ray.origin + ray.direction * (distance), R };
            secondColor = pixelShader(rayCast(scene, secondRay), scene.light, secondRay);
         }
         else
            secondColor = firstColor;
         float mix = (1.f - rough) * met;
         Color color = firstColor * (1.f - mix) + secondColor * mix;
         color = { std::clamp(color.r, 0.f, 1.f) ,std::clamp(color.g, 0.f, 1.f) ,std::clamp(color.b, 0.f, 1.f) };
         bitmap.SetPixel(j, i, {
                            static_cast<uint8_t>(color.r * 255.f), static_cast<uint8_t>(color.g * 255.f),
                            static_cast<uint8_t>(color.b * 255.f)
                         });
      }
   bitmap.Save(filename);
}
