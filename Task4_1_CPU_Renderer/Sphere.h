#pragma once
#include "Object.h"

class Sphere :
   public Object
{
   Point3D center;
   float r;
   float r2;
public:
   Sphere() = default;

   Sphere(const Point3D& center, float r, const Color& diffuseColor, const Color& specularColor, float specularExp,
          float metalness, float roughness);

   std::pair<Point3D, float> closestHit(Ray ray) override;
   bool anyHit(Ray ray) override;

   virtual ~Sphere() = default;
};
