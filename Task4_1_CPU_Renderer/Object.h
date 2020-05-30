#pragma once
#include <tuple>
#include "Structures.h"

class Object
{
protected:
   Color diffuseColor;
   Color specularColor;
   float specularExp;
   float metalness;
   float roughness;

   Object(const Color& diffuseColor, const Color& specularColor, float specularExp, float metalness, float roughness)
      : diffuseColor(diffuseColor),
        specularColor(specularColor),
        specularExp(specularExp),
        metalness(metalness),
        roughness(roughness)
   {
   }

   Object() = default;
public:
   virtual std::pair<Point3D, float> closestHit(Ray ray) = 0;
   virtual bool anyHit(Ray ray) = 0;

   [[nodiscard]] const Color& getDiffuseColor() const;
   [[nodiscard]] const Color& getSpecularColor() const;
   [[nodiscard]] float getSpecularExp() const;
   [[nodiscard]] float getMetalness() const;
   [[nodiscard]] float getRoughness() const;

   virtual ~Object() = default;
};


inline const Color& Object::getDiffuseColor() const
{
   return diffuseColor;
}

inline const Color& Object::getSpecularColor() const
{
   return specularColor;
}

inline float Object::getSpecularExp() const
{
   return specularExp;
}

inline float Object::getMetalness() const
{
   return metalness;
}

inline float Object::getRoughness() const
{
   return roughness;
}
