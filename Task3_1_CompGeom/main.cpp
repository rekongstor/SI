#include <random>
#include <stack>

#define N 100
#define SIZE_X 100
#define SIZE_Y 100

#define MIN_X 0.f
#define MAX_X 100.f
#define MIN_Y 0.f
#define MAX_Y 100.f

constexpr float DIST_X = MAX_X - MIN_X;
constexpr float DIST_Y = MAX_Y - MIN_Y;


struct Vertex
{
   float x;
   float y;
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

struct Edge
{
   Vertex* v0;
   Vertex* v1;
};

struct EdgeAdj
{
   Vertex* v0; // main
   Vertex* v1; // adj
   Vertex* v2; // main
   Vertex* v3; // adj

   EdgeAdj(Vertex* v0, Vertex* v1, Vertex* v2) : v0(v0), v1(v1), v2(v2)
   {
   }

   void Add(Vertex V3)
   {
   }

   void CheckAndFlip()
   {
      Vertex V0 = *v0;
      Vertex V1 = *v1;
      Vertex V2 = *v2;
      Vertex V3 = *v3;
      if (
         ((V0.x - V1.x) * (V0.y - V3.y) - (V0.x - V3.x) * (V0.y - V1.y)) *
         ((V2.x - V1.x) * (V2.x - V3.x) + (V2.y - V1.y) * (V2.y - V3.y))
         +
         ((V0.x - V1.x) * (V0.x - V3.x) + (V0.y - V1.y) * (V0.y - V3.y)) *
         ((V2.x - V1.x) * (V2.y - V3.y) - (V2.x - V3.x) * (V2.y - V1.y))
         < 0.f)
      {
         std::swap(v0, v1);
         std::swap(v2, v3);
      }
   }
};


void Delaunay(std::vector<Vertex>& vertices, std::vector<EdgeAdj> edges)
{
   std::sort(vertices.begin(), vertices.end(), [](const Vertex& a, const Vertex& b) -> bool
   {
      return a.x < b.x;
   });
   edges.emplace_back(EdgeAdj(&vertices[0], &vertices[1], &vertices[2]));
   edges.emplace_back(EdgeAdj(&vertices[1], &vertices[2], &vertices[0]));
   edges.emplace_back(EdgeAdj(&vertices[2], &vertices[0], &vertices[1]));
}

void Graham(std::vector<Vertex>& vertices, std::vector<EdgeAdj> edges)
{
   // Finding rightmost lowest point O(n)
   Vertex* plowRight = &vertices.front();

   for (int i = 1; i < vertices.size(); ++i)
      if (vertices[i].y < plowRight->y ||
         (vertices[i].y == plowRight->y && vertices[i].x > plowRight->x))
         plowRight = &vertices[i];
   // Sort by angle
   Vertex lowRight = *plowRight;
   std::swap(vertices.front(), *plowRight);

   auto it = vertices.begin() + 1;
   std::sort(it, vertices.end(), [&](const Vertex& a, const Vertex& b) -> bool
   {
      if (Area2(lowRight, a, b) > 0.f)
         return false;
      return true;
   });

   // Remove collinear
   std::vector<Vertex*> sortedVertices;
   sortedVertices.reserve(vertices.size());
   int cursor = 0;
   for (int i = 1; i < vertices.size() - 1; ++i)
   {
      if (abs(Area2(lowRight, vertices[i], vertices[i + 1])) <= std::numeric_limits<float>::epsilon())
         // area is almost 0
      {
         if (Dist2(lowRight, vertices[i]) < Dist2(lowRight, vertices[i + 1]))
            sortedVertices.back() = &vertices[i + 1]; // if [i + 1] is more distant, then replace [cursor] with it
      }
      else
      {
         sortedVertices.push_back(&vertices[i]);
      }
   }

   // Initializing stack
   std::stack<Vertex*> hullStack;
   hullStack.push(&vertices.front());
   hullStack.push(sortedVertices.front());
   int i = 2;
   while (i < sortedVertices.size())
   {
      if (hullStack.size() == 1)
      {
         hullStack.push(sortedVertices[i]);
         ++i;
         continue;
      }
      Vertex* p1 = hullStack.top();
      hullStack.pop();
      Vertex* p0 = hullStack.top();
      if (!Left(*p0, *p1, *sortedVertices[i]))
      {
         hullStack.push(p1);
         hullStack.push(sortedVertices[i]);
         ++i;
      }
   }
   return;
}


int main()
{
   // Step 0: Generation
   std::mt19937 engine;
   std::uniform_real_distribution<float> realDistribution;
   std::vector<Vertex> vertices(N);
   for (auto& v : vertices)
      v = {realDistribution(engine) * DIST_X + MIN_X, realDistribution(engine) * DIST_Y + MIN_Y};


   // Step 1: Convex hull
   std::vector<EdgeAdj> edgesHull;
   Graham(vertices, edgesHull);

   // Step 2: Delaunay
   std::vector<EdgeAdj> edgesTriangulation;
   Delaunay(vertices, edgesTriangulation);

   // Step 3: BMP
   return 0;
}
