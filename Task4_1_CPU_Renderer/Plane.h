#pragma once
#include "Object.h"

class Plane :
   public Object
{
   Point4D L;
   Color diffuseColor;
   Color specularColor;
   float specularExp;
public:
   Plane(Point4D L, const Color& diffuseColor, const Color& specularColor, float specularExp);

   std::pair<Point3D, float> closestHit(Ray ray) override;
   bool anyHit(Ray ray) override;
   std::tuple<Color, Color, float> getColor() override;
};
