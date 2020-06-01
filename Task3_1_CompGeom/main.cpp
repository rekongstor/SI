#include <random>
#include <list>
#include "../Core/Bitmap.h"
#include "Structures.h"

#define N 100
#define SIZE_X 1600
#define SIZE_Y 900

#define MIN_X 50.f
#define MAX_X 1550
#define MIN_Y 50.f
#define MAX_Y 850

constexpr float DIST_X = MAX_X - MIN_X;
constexpr float DIST_Y = MAX_Y - MIN_Y;


void Graham(std::vector<Vertex>& vertices, std::list<Edge>& hullEdges, std::vector<Vertex>& hullVertices);
void Delaunay(std::vector<Vertex>& vertices, std::list<EdgeAdj>& edges);


void MakeBMP(std::vector<Vertex>& vertices, std::list<EdgeAdj>& edges, std::list<Edge>& hull)
{
   Bitmap bmp(SIZE_X, SIZE_Y);
   for (auto& e : edges)
   {
      bmp.DrawLine(e.v0->x, e.v0->y, e.v1->x, e.v1->y);
   }
   for (auto& e : hull)
   {
      bmp.DrawLine(e.v0.x, e.v0.y, e.v1.x, e.v1.y,0,255,255);
   }
   for (auto& v : vertices)
   {
      bmp.DrawPoint(v.x, v.y);
   }
   bmp.Save("triangulation.bmp");
}



int main()
{
   // Step 0: Generation
   std::mt19937 engine(1337);
   std::uniform_real_distribution<float> realDistribution;
   std::vector<Vertex> vertices(N);
   for (auto& v : vertices)
      v = {realDistribution(engine) * DIST_X + MIN_X, realDistribution(engine) * DIST_Y + MIN_Y};


   // Step 1: Convex hull
   std::list<Edge> hullEdges;
   std::vector<Vertex> hullVertices;
   Graham(vertices, hullEdges, hullVertices);

   // Step 2: Delaunay
   std::list<EdgeAdj> edgesTriangulation;
   Delaunay(hullVertices, edgesTriangulation);

   // Step 3: BMP
   MakeBMP(vertices, edgesTriangulation, hullEdges);
   return 0;
}
