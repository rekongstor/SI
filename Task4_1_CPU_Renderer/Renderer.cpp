#define _USE_MATH_DEFINES
#include "Renderer.h"
#include "Scene.h"
#include "../Core/Bitmap.h"


Renderer::Renderer(const Camera& camera, const Color& ambientColor, const Color& voidColor) :
   camera(camera),
   ambientColor(ambientColor),
   voidColor(voidColor)
{
}


constantBuffer Renderer::rayCast(Scene& scene, Ray ray)
{
   // Finding closest pixel color
   std::pair<Point3D, float> closest;
   closest.second = std::numeric_limits<float>::infinity();
   Color diffuseColor = Color(), specularColor = Color();
   float specularExp = 0.f, metalness = 0.f, roughness = 0.f;
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
         return {black, specularColor, specularExp, metalness, roughness, closest.first, closest.second};
   }
   return {diffuseColor, specularColor, specularExp, metalness, roughness, closest.first, closest.second};
}

void Renderer::renderScene(Scene& scene, uint32_t width, uint32_t height, const char* filename)
{
   std::vector<Ray> rays;
   rays.resize(width * height);

   // Calculating basis
   Point3D up = {0.f, 0.f, 1.f};
   Point3D u;
   if (length2(up - camera.direction) > 0.01f)
   {
      u = normalize(mul(camera.direction, up));
   }
   else
   {
      u = {1.f, 0.f, 0.f};
   }
   Point3D w = mul(camera.direction, u);

   // Calculating plane
   Point3D left = rotate(camera.direction, w, camera.fov * M_PI / 180.f);
   Point3D right = rotate(camera.direction, w, -camera.fov * M_PI / 180.f);
   float side = -sqrtf(2.f / (1.f - cosf(camera.fov * M_PI / 180.f)));
   left = left * side;
   right = right * side;
   side = -side;
   float planeDistance = length(mul(left, right)) * 0.25;
   Point3D uwPosition = camera.position + (camera.direction * planeDistance);
   uwPosition = uwPosition - (u + w * (static_cast<float>(height) / static_cast<float>(width)));
   // moving to (-1;-a) uw coordinate where a is an aspect ratio

   float delta = 2.f / static_cast<float>(width);
   Point3D deltaX = u * delta;
   Point3D deltaY = w * delta;

   // Creating rays
   for (auto j = 0; j < height; ++j)
   {
      for (auto i = 0; i < width; ++i)
      {
         rays[j * width + i] = {{uwPosition}, {normalize(uwPosition - camera.position)}};
         uwPosition = uwPosition + deltaX;
      }
      uwPosition = uwPosition - (deltaX) * width + deltaY;
   }

   // Calculating rays
   Bitmap bitmap(width, height);
   for (auto j = 0; j < height; ++j)
      for (auto i = 0; i < width; ++i)
      {
         // First ray
         Ray firstRay = rays[j * width + i];
         auto rayBuffer = rayCast(scene, firstRay);
         Color firstColor = pixelShader(rayBuffer, scene.light, firstRay);
         Color secondColor = firstColor;
         if (length2(rayBuffer.normal) > 0.9f)
         {
            Point3D L = firstRay.direction * -1.f;
            Point3D R = rayBuffer.normal * 2.f * dot(rayBuffer.normal, L) - L;
            // Second ray for reflection
            Ray secondRay = {firstRay.origin + firstRay.direction * (rayBuffer.distance), R};
            secondColor = pixelShader(rayCast(scene, secondRay), scene.light, secondRay);
         }

         // Reflection mixing factor 
         float mix = (1.f - rayBuffer.roughness) * rayBuffer.metalness;

         Color color = firstColor * (1.f - mix) + secondColor * mix;

         // Clamping color and writing the pixel
         color = {std::clamp(color.r, 0.f, 1.f), std::clamp(color.g, 0.f, 1.f), std::clamp(color.b, 0.f, 1.f)};
         bitmap.SetPixel(i, j, {
                            static_cast<uint8_t>(color.r * 255.f), static_cast<uint8_t>(color.g * 255.f),
                            static_cast<uint8_t>(color.b * 255.f)
                         });
      }
   bitmap.Save(filename);
}
