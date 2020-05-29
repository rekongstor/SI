#pragma once
#include <vector>

#include "Object.h"

class Scene
{
   Camera camera;
   Light light;
   std::vector<Object*> objects;

   std::tuple<Color, Color, float, Point3D> getPixelColor(Ray ray);
   Color getPixelColorPhong(Ray ray);

public:
   explicit Scene(const Camera& camera, const Light& light);

   void addObject(Object* object);
   void renderScene(uint32_t width, uint32_t height, const char* filename);
};
