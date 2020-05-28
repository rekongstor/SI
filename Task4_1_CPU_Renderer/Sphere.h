#pragma once
#include "Object.h"

class Sphere :
   public Object
{
   Point3D center;
   Color diffuseColor;
   Color specularColor;
   float specularExp;
public:
   std::tuple<Color, Color, float> getColor() override;
   Sphere(const Point3D& center, float r,
          Color diffuseColor = {1.f, 0.f, 0.f}, Color specularColor = {0.5f, 0.5f, 0.f}, float specularExp = 10.f);

private:
   float r;
   float r2;
public:
   std::pair<Point3D, float> closestHit(Ray ray) override;
   bool anyHit(Ray ray) override;
};
