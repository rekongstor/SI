#include "Sphere.h"


Sphere::Sphere(const Point3D& center, float r, const Color& diffuseColor, const Color& specularColor, float specularExp,
               float metalness, float roughness):
   Object(diffuseColor, specularColor, specularExp, metalness, roughness),
   center(center),
   r(r),
   r2(r * r)
{
}

std::pair<Point3D, float> Sphere::closestHit(Ray ray)
{
   Point3D direction = ray.direction;
   Point3D origin = ray.origin - center;

   float a = dot(direction, direction);
   float b = 2.f * dot(origin, direction);
   float c = dot(origin, origin) - r2;

   float D = b * b - 4 * a * c;
   float t = (-b - sqrtf(D)) / (2.f * a);

   Point3D normal = (origin + direction * t) / r;
   return {normal, t};
}

bool Sphere::anyHit(Ray ray)
{
   Point3D direction = ray.direction;
   Point3D origin = ray.origin - center;
   if (dot(origin - center, origin - center) < r2)
      return false;

   float a = dot(direction, direction);
   float b = 2.f * dot(origin, direction);
   float c = dot(origin, origin) - r2;

   float D = b * b - 4 * a * c;
   float t = (-b - sqrtf(D)) / (2.f * a);
   if (D < 0 || t < 0)
      return false;
   return true;
}
