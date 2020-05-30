#pragma once
#include <algorithm>

struct Point3D
{
   float x, y, z;
};

struct Point4D
{
   float x, y, z, w;
};

struct Quaternion
{
   float w, x, y, z;
};

struct Ray
{
   Point3D origin;
   Point3D direction;
};

struct Color
{
   float r, g, b;
};

struct Light
{
   Point3D direction;
   Color color;
};

struct Camera
{
   Point3D position;
   Point3D direction;
   float fov;
};

inline float dot(const Point3D& v1, const Point3D& v2)
{
   return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

inline float dot(const Quaternion& q1, const Quaternion& q2)
{
   return q1.w * q2.w + q1.x * q2.x + q1.y * q2.y + q1.z * q2.z;
}

inline float dot(const Point4D v1, const Point4D& v2)
{
   return v1.w * v2.w + v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

inline Point3D mul(const Point3D& v1, const Point3D& v2)
{
   return {v1.y * v2.z - v1.z * v2.y, -(v1.x * v2.z - v1.z * v2.x), v1.x * v2.y - v1.y * v2.x};
}

inline Quaternion mul(const Quaternion& a, const Point3D& b)
{
   return {
      -a.x * b.x - a.y * b.y - a.z * b.z,
      a.w * b.x + a.y * b.z - a.z * b.y,
      a.w * b.y - a.x * b.z + a.z * b.x,
      a.w * b.z + a.x * b.y - a.y * b.x
   };
}

inline float length(const Point3D& v)
{
   return sqrtf(dot(v, v));
}

inline float length(const Quaternion& v)
{
   return sqrtf(dot(v, v));
}

inline Quaternion mul(const Quaternion& a, const Quaternion& b)
{
   return {
      a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
      a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
      a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
      a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w
   };
}

inline Point3D normalize(Point3D vector)
{
   float len = length(vector);
   vector.x /= len;
   vector.y /= len;
   vector.z /= len;
   return vector;
}

inline Quaternion normalize(Quaternion vector)
{
   float len = length(vector);
   vector.w /= len;
   vector.x /= len;
   vector.y /= len;
   vector.z /= len;
   return vector;
}

inline Point3D rotate(const Point3D& vector, const Point3D& axis, const float angle)
{
   Quaternion q;
   q.w = cosf(angle / 2.f);
   float sinAngle = sinf(angle / 2.f);
   q.x = axis.x * sinAngle;
   q.y = axis.y * sinAngle;
   q.z = axis.z * sinAngle;
   q = normalize(q);

   Quaternion qRev = q;
   qRev.x = -qRev.x;
   qRev.y = -qRev.y;
   qRev.z = -qRev.z;

   Quaternion x = mul(mul(q, vector), qRev);
   return {x.x, x.y, x.z};
}

inline Point3D operator+(const Point3D& l, const Point3D& r)
{
   return {l.x + r.x, l.y + r.y, l.z + r.z};
}

inline Point3D operator-(const Point3D& l, const Point3D& r)
{
   return {l.x - r.x, l.y - r.y, l.z - r.z};
}


inline Point3D operator+(const Point3D& l, const float r)
{
   return {l.x + r, l.y + r, l.z + r};
}

inline Point3D operator+(const float l, const Point3D& r)
{
   return r + l;
}

inline Point3D operator-(const Point3D& l, const float r)
{
   return {l.x - r, l.y - r, l.z - r};
}

inline Point3D operator-(const float l, const Point3D& r)
{
   return r - l;
}

inline Point3D operator*(const Point3D& l, const float r)
{
   return {l.x * r, l.y * r, l.z * r};
}

inline Point3D operator*(const float l, const Point3D& r)
{
   return r * l;
}

inline Point3D operator/(const Point3D& l, const float r)
{
   return {l.x / r, l.y / r, l.z / r};
}

inline Point3D operator/(const float l, const Point3D& r)
{
   return r / l;
}


inline Color operator+(const Color& l, const Color& r)
{
   return {l.r + r.r, l.g + r.g, l.b + r.b};
}

inline Color operator-(const Color& l, const Color& r)
{
   return { l.r - r.r, l.g - r.g, l.b - r.b };
}

inline Color operator*(const Color& l, const Color& r)
{
   return { l.r * r.r, l.g * r.g, l.b * r.b };
}

inline Color operator*(const Color& l, const float r)
{
   return { l.r * r, l.g * r, l.b * r };
}

inline Color operator/(const Color& l, const float r)
{
   return { l.r / r, l.g / r, l.b / r };
}
