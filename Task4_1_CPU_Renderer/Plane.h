#pragma once
#include "Object.h"

class Plane :
   public Object
{
   Point4D L;
public:
   Plane() = default;
   Plane(const Point4D& l, const Color& diffuseColor, const Color& specularColor, float specularExp,
         float metalness, float roughness);

   std::pair<Point3D, float> closestHit(Ray ray) override;
   bool anyHit(Ray ray) override;

   virtual ~Plane() = default;
};
