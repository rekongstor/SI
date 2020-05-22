#pragma once


struct Vertex
{
   float x;
   float y;
};

struct EdgeAdj;

struct Triangle
{
   EdgeAdj* e0;
   EdgeAdj* e1;
   EdgeAdj* e2;
};

struct EdgeAdj
{
   Vertex* v0;
   Vertex* v1;
   Triangle* left;
   Triangle* right;
};

struct Edge
{
   Vertex v0;
   Vertex v1;
};

inline float Dist2(Vertex v0, Vertex v1)
{
   float x = v0.x - v1.x;
   float y = v0.y - v1.y;
   return x * x + y * y;
}

inline float Area2(Vertex v0, Vertex v1, Vertex v2)
{
   return (v1.x - v0.x) * (v2.y - v0.y) - (v2.x - v0.x) * (v1.y - v0.y);
}

inline bool Left(Vertex v0, Vertex v1, Vertex v2)
{
   return Area2(v0, v1, v2) > 0;
}