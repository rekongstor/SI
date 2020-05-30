#include "Scene.h"
#include "../Core/Bitmap.h"

Scene::Scene(const Light& light): light(light)
{
}


void Scene::addObject(Object* object)
{
   objects.push_back(object);
}

