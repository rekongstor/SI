#pragma once
#include <tuple>
#include "Structures.h"

class Object
{
public:
   virtual std::pair<Point3D, float> closestHit(Ray ray) = 0;
   virtual bool anyHit(Ray ray) = 0;
   virtual std::tuple<Color, Color, float> getColor() = 0;
   virtual ~Object() = default;
};