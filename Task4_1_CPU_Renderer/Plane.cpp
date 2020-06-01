#include "Plane.h"


Plane::Plane(const Point4D& L, const Color& diffuseColor, const Color& specularColor, float specularExp,
             float metalness, float roughness):
   Object(diffuseColor, specularColor, specularExp, metalness, roughness),
   L({L.x, L.y, -L.z, L.w})
{
}

std::pair<Point3D, float> Plane::closestHit(Ray ray)
{
   Point4D S = {ray.origin.x, ray.origin.y, ray.origin.z, 1.f};
   Point4D V = {ray.direction.x, ray.direction.y, ray.direction.z, 0.f};
   float t = -dot(L, S) / dot(L, V);
   return {{L.x, L.y, -L.z}, t};
}

bool Plane::anyHit(Ray ray)
{
   Point4D S = {ray.origin.x, ray.origin.y, ray.origin.z, 1.f};
   Point4D V = {ray.direction.x, ray.direction.y, ray.direction.z, 0.f};
   if (dot(L, V) < 0.001f)
      return false;
   float t = -dot(L, S) / dot(L, V);
   if (t < 0.f)
      return false;
   return true;
}
