#include "Sphere.h"

Sphere::Sphere(const Point3D& center, float r, Color diffuseColor, Color specularColor, float specularExp) : center(center),
                                                 diffuseColor(diffuseColor),
                                                 specularColor(specularColor),
                                                 specularExp(specularExp),
                                                 r(r),
                                                 r2(r * r)
{
}

std::tuple<Color, Color, float> Sphere::getColor()
{
   return {diffuseColor, specularColor, specularExp};
}

std::pair<Point3D, float> Sphere::closestHit(Ray ray)
{
   Point3D direction = ray.direction;
   Point3D origin = ray.origin;

   float a = dot(direction, direction);
   float b = 2.f * dot(origin, direction);
   float c = dot(origin,origin) - r2;

   float D = b * b - 4 * a * c;
   float t = (-b - sqrtf(D)) / (2.f * a);

   Point3D normal = {
      (origin.x + t * direction.x - center.x) / r, (origin.y + t * direction.y - center.y) / r,
      (origin.z + t * direction.z - center.z) / r
   };
   return {normal, t};
}

bool Sphere::anyHit(Ray ray)
{
   Point3D direction = ray.direction;
   Point3D origin = ray.origin;

   float a = dot(direction, direction);
   float b = 2.f * dot(origin, direction);
   float c = dot(origin, origin) - r2;
   return b * b - 4 * a * c >= 0;
}
