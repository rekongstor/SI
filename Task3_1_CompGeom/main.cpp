#include <random>
#include <stack>
#include <list>

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


   void CheckAndFlip()
   {
      // Edge
      Vertex V1 = *v1;
      Vertex V3 = *v0;

      // Left triangle
      Vertex* lv0 = left->e0->v0;
      Vertex* lv1 = left->e0->v1;
      Vertex* lv2 = (left->e1->v0 != lv0 && left->e1->v0 != lv1) ? left->e1->v0 : left->e1->v1;

      Vertex* lVertex;
      if (lv0 != v0 && lv0 != v1)
         lVertex = lv0;
      else if (lv1 != v0 && lv1 != v1)
         lVertex = lv1;
      else
         lVertex = lv2;

      // Right triangle
      Vertex* rv0 = right->e0->v0;
      Vertex* rv1 = right->e0->v1;
      Vertex* rv2 = (right->e1->v0 != rv0 && right->e1->v0 != rv1) ? right->e1->v0 : right->e1->v1;

      Vertex* rVertex;
      if (rv0 != v0 && rv0 != v1)
         rVertex = rv0;
      else if (rv1 != v0 && rv1 != v1)
         rVertex = rv1;
      else
         rVertex = rv2;

      Vertex V0 = *lVertex;
      Vertex V2 = *rVertex;

      if (
         ((V0.x - V1.x) * (V0.y - V3.y) - (V0.x - V3.x) * (V0.y - V1.y)) *
         ((V2.x - V1.x) * (V2.x - V3.x) + (V2.y - V1.y) * (V2.y - V3.y))
         +
         ((V0.x - V1.x) * (V0.x - V3.x) + (V0.y - V1.y) * (V0.y - V3.y)) *
         ((V2.x - V1.x) * (V2.y - V3.y) - (V2.x - V3.x) * (V2.y - V1.y))
         < 0.f)
      {
         // Flip this edge
         v0 = lVertex;
         v1 = rVertex;

         // Flip triangles
         EdgeAdj *le1, *le2;
         if (left->e0 == this)
         {
            le1 = left->e1;
            le2 = left->e2;
         }
         else if (left->e1 == this)
         {
            le1 = left->e2;
            le2 = left->e0;
         } else
         {
            le1 = left->e0;
            le2 = left->e1;
         }

         EdgeAdj* re1, * re2;
         if (right->e0 == this)
         {
            re1 = right->e1;
            re2 = right->e2;
         }
         else if (right->e1 == this)
         {
            re1 = right->e2;
            re2 = right->e0;
         }
         else
         {
            re1 = right->e0;
            re2 = right->e1;
         }

         left->e0 = this;
         left->e1 = re2;
         left->e2 = le1;

         right->e0 = this;
         right->e1 = le2;
         right->e2 = re1;

         le2->left = right;
         re2->left = left;
      }
   }
};


void Delaunay(std::vector<Vertex>& vertices, std::list<EdgeAdj>& edges)
{
   std::sort(vertices.begin(), vertices.end(), [](const Vertex& a, const Vertex& b) -> bool
   {
      return a.x < b.x;
   });
   std::list<Triangle> triangles;
   std::list<EdgeAdj*> hull;

   // Adding one triangle, its edges and its convex hull
   if (Left(vertices[0], vertices[1], vertices[2]))
   {
      triangles.push_back({});
      edges.push_back({&vertices[0], &vertices[1], &triangles.back()});
      hull.push_back(&edges.back());
      triangles.front().e0 = &edges.back();
      edges.push_back({&vertices[1], &vertices[2], &triangles.back()});
      hull.push_back(&edges.back());
      triangles.front().e1 = &edges.back();
      edges.push_back({&vertices[2], &vertices[0], &triangles.back()});
      hull.push_back(&edges.back());
      triangles.front().e2 = &edges.back();
   }
   else
   {
      triangles.push_back({});
      edges.push_back({&vertices[1], &vertices[0], &triangles.back()});
      hull.push_back(&edges.back());
      triangles.front().e0 = &edges.back();
      edges.push_back({&vertices[0], &vertices[2], &triangles.back()});
      hull.push_back(&edges.back());
      triangles.front().e1 = &edges.back();
      edges.push_back({&vertices[2], &vertices[1], &triangles.back()});
      hull.push_back(&edges.back());
      triangles.front().e2 = &edges.back();
   }

   for (int i = 3; i < vertices.size(); ++i)
   {
      // Finding visible start and end
      auto start = hull.begin();
      bool counterClockwise = Left(*(*start)->v0, *(*start)->v1, vertices[i]);
      do
      {
         if (counterClockwise)
         {
            ++start;
            if (!Left(*(*start)->v0, *(*start)->v1, vertices[i]))
               break;
         }
         else
         {
            if (!Left(*(*start)->v0, *(*start)->v1, vertices[i]))
               break;
            --start;
         }
      }
      while (start != hull.begin());

      auto end = counterClockwise ? start : hull.begin();
      while (!Left(*(*end)->v0, *(*end)->v1, vertices[i]))
      {
         ++end;
      }

      // Adding vertices to visible edges
      EdgeAdj* first = nullptr, * second;
      for (auto it = start; it != end; ++it)
      {
         auto& edge = *it;
         triangles.push_back({edge});
         edge->right = &triangles.back();
         edges.push_back({edge->v0, &vertices[i], &triangles.back()});
         if (first == nullptr)
            first = &edges.back();
         triangles.back().e1 = &edges.back();
         edges.push_back({&vertices[i], edge->v1, &triangles.back()});
         second = &edges.back();
         triangles.back().e2 = &edges.back();
         edge->CheckAndFlip();
      }
      hull.insert(start, first);
      hull.insert(start, second);
      hull.erase(start, end);
   }
}

void Graham(std::vector<Vertex>& vertices, std::stack<Vertex*>& hullStack)
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
   std::stack<Vertex*> hullStack;
   Graham(vertices, hullStack);

   // Step 2: Delaunay
   std::list<EdgeAdj> edgesTriangulation;
   Delaunay(vertices, edgesTriangulation);

   // Step 3: BMP
   return 0;
}
