#include <algorithm>
#include <cmath>
#include <list>
#include <vector>

#include "Structures.h"


void CheckAndFlip(EdgeAdj* edge)
{
   if (edge->right == nullptr || edge->left == nullptr)
      return;
   // Edge
   Vertex V1 = *edge->v1;
   Vertex V3 = *edge->v0;

   // Left triangle
   Vertex* leftTriangleV0 = edge->left->e0->v0;
   Vertex* leftTriangleV1 = edge->left->e0->v1;
   Vertex* leftTriangleV2 = (edge->left->e1->v0 != leftTriangleV0 && edge->left->e1->v0 != leftTriangleV1) ? edge->left->e1->v0 : edge->left->e1->v1;

   Vertex* leftAdjVertex;
   if (leftTriangleV0 != edge->v0 && leftTriangleV0 != edge->v1)
      leftAdjVertex = leftTriangleV0;
   else if (leftTriangleV1 != edge->v0 && leftTriangleV1 != edge->v1)
      leftAdjVertex = leftTriangleV1;
   else
      leftAdjVertex = leftTriangleV2;



   // Right triangle
   Vertex* rightTriangleV0 = edge->right->e0->v0;
   Vertex* rightTriangleV1 = edge->right->e0->v1;
   Vertex* rightTriangleV2 = (edge->right->e1->v0 != rightTriangleV0 && edge->right->e1->v0 != rightTriangleV1) ? edge->right->e1->v0 : edge->right->e1->v1;


   Vertex* rightAdjVertex;
   if (rightTriangleV0 != edge->v0 && rightTriangleV0 != edge->v1)
      rightAdjVertex = rightTriangleV0;
   else if (rightTriangleV1 != edge->v0 && rightTriangleV1 != edge->v1)
      rightAdjVertex = rightTriangleV1;
   else
      rightAdjVertex = rightTriangleV2;

   Vertex V0 = *leftAdjVertex;
   Vertex V2 = *rightAdjVertex;

   if (Left(V0, V1, V2) || Left(V0, V2, V3))
      return;

   if (
      abs(((V0.x - V1.x) * (V0.y - V3.y) - (V0.x - V3.x) * (V0.y - V1.y))) *
      ((V2.x - V1.x) * (V2.x - V3.x) + (V2.y - V1.y) * (V2.y - V3.y))
      +
      ((V0.x - V1.x) * (V0.x - V3.x) + (V0.y - V1.y) * (V0.y - V3.y)) *
      abs(((V2.x - V1.x) * (V2.y - V3.y) - (V2.x - V3.x) * (V2.y - V1.y)))
      < 0.f)
   {
      // Flip this edge
      edge->v0 = leftAdjVertex;
      edge->v1 = rightAdjVertex;

      // Flip triangles
      EdgeAdj* leftTriangleEdge1, * leftTriangleEdge2;
      if (edge->left->e0 == edge)
      {
         leftTriangleEdge1 = edge->left->e1;
         leftTriangleEdge2 = edge->left->e2;
      }
      else if (edge->left->e1 == edge)
      {
         leftTriangleEdge1 = edge->left->e2;
         leftTriangleEdge2 = edge->left->e0;
      }
      else
      {
         leftTriangleEdge1 = edge->left->e0;
         leftTriangleEdge2 = edge->left->e1;
      }

      EdgeAdj* rightTriangleEdge1, * rightTriangleEdge2;
      if (edge->right->e0 == edge)
      {
         rightTriangleEdge1 = edge->right->e1;
         rightTriangleEdge2 = edge->right->e2;
      }
      else if (edge->right->e1 == edge)
      {
         rightTriangleEdge1 = edge->right->e2;
         rightTriangleEdge2 = edge->right->e0;
      }
      else
      {
         rightTriangleEdge1 = edge->right->e0;
         rightTriangleEdge2 = edge->right->e1;
      }

      edge->left->e0 = edge;
      edge->left->e1 = rightTriangleEdge2;
      edge->left->e2 = leftTriangleEdge1;

      edge->right->e0 = edge;
      edge->right->e1 = leftTriangleEdge2;
      edge->right->e2 = rightTriangleEdge1;

      if (leftTriangleEdge1->left == edge->right)
         leftTriangleEdge1->left = edge->left;
      if (leftTriangleEdge1->right == edge->right)
         leftTriangleEdge1->right = edge->left;

      if (leftTriangleEdge2->left == edge->left)
         leftTriangleEdge2->left = edge->right;
      if (leftTriangleEdge2->right == edge->left)
         leftTriangleEdge2->right = edge->right;

      if (rightTriangleEdge1->left == edge->left)
         rightTriangleEdge1->left = edge->right;
      if (rightTriangleEdge1->right == edge->left)
         rightTriangleEdge1->right = edge->right;

      if (rightTriangleEdge2->left == edge->right)
         rightTriangleEdge2->left = edge->left;
      if (rightTriangleEdge2->right == edge->right)
         rightTriangleEdge2->right = edge->left;

      CheckAndFlip(edge->left->e1);
      CheckAndFlip(edge->left->e2);
      CheckAndFlip(edge->right->e1);
      CheckAndFlip(edge->right->e2);
   }
}



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
      edges.push_back({ &vertices[0], &vertices[1], &triangles.back() });
      hull.push_back(&edges.back());
      triangles.front().e0 = &edges.back();
      edges.push_back({ &vertices[1], &vertices[2], &triangles.back() });
      hull.push_back(&edges.back());
      triangles.front().e1 = &edges.back();
      edges.push_back({ &vertices[2], &vertices[0], &triangles.back() });
      hull.push_back(&edges.back());
      triangles.front().e2 = &edges.back();
   }
   else
   {
      triangles.push_back({});
      edges.push_back({ &vertices[1], &vertices[0], &triangles.back() });
      hull.push_back(&edges.back());
      triangles.front().e0 = &edges.back();
      edges.push_back({ &vertices[0], &vertices[2], &triangles.back() });
      hull.push_back(&edges.back());
      triangles.front().e1 = &edges.back();
      edges.push_back({ &vertices[2], &vertices[1], &triangles.back() });
      hull.push_back(&edges.back());
      triangles.front().e2 = &edges.back();
   }

   for (int i = 3; i < vertices.size(); ++i)
   {
      // Finding visible start and end
      auto start = hull.begin();
      bool counterClockwise = Left(*(*start)->v0, *(*start)->v1, vertices[i]);
      if (!counterClockwise)
         start = hull.end();
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
            --start;
            if (Left(*(*start)->v0, *(*start)->v1, vertices[i]))
            {
               ++start;
               if (start == hull.end())
                  start = hull.begin();
               break;
            }
         }
      } while (true);

      auto end = counterClockwise ? start : hull.begin();
      while (!Left(*(*end)->v0, *(*end)->v1, vertices[i]))
      {
         ++end;
         if (end == hull.end())
         {
            end = hull.begin();
         }
      }

      bool endIsCycled = false;
      for (auto it = start; it != end; ++it)
         if (it == hull.end())
         {
            endIsCycled = true;
            break;
         }

      // Adding vertices to visible edges
      EdgeAdj* first = nullptr, * second = nullptr;
      bool singleEdge;
      for (auto it = start; it != end; ++it)
      {
         if (it == hull.end())
         {
            it = hull.begin();
            if (it == end)
               break;
         }
         auto& edge = *it;
         triangles.push_back({ edge });
         edge->right = &triangles.back();
         if (second == nullptr)
            edges.push_back({ edge->v0, &vertices[i], &triangles.back() });
         else
            edges.back().right = &triangles.back();
         if (first == nullptr)
            first = &edges.back();
         triangles.back().e1 = &edges.back();
         edges.push_back({ &vertices[i], edge->v1, &triangles.back() });
         second = &edges.back();
         triangles.back().e2 = &edges.back();
         // Check Delaunay condition
         CheckAndFlip(edge);
      }

      // Update hull

      if (!endIsCycled)
      {
         hull.insert(start, first);
         hull.insert(start, second);
         hull.erase(start, end);
      }
      else
      {
         hull.insert(start, first);
         hull.insert(start, second);
         hull.erase(start, hull.end());
         hull.erase(hull.begin(), end);
      }

   }
}
