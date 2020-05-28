#pragma once
#include <vector>

#include "Object.h"

class Scene
{
   Camera camera;
   std::vector<Object*> objects;
   std::vector<Light> lights; // we assume there are only directional lights

   Color getPixelColorPhong(Ray ray);

public:
   explicit Scene(const Camera& camera);

   void addObject(Object* object);
   void renderScene(uint32_t width, uint32_t height, const char* filename);
};
