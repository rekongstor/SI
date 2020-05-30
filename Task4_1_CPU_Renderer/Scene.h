#pragma once
#include <vector>

#include "Object.h"

class Renderer;

class Scene
{
   friend class Renderer;
   Light light;
   std::vector<Object*> objects;

public:
   explicit Scene(const Light& light);

   void addObject(Object* object);
};
