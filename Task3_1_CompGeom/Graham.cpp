#include <algorithm>
#include <list>
#include <stack>
#include <vector>

#include "Structures.h"


void Graham(std::vector<Vertex>& vertices, std::list<Edge>& hullEdges)
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
   sortedVertices.push_back(&vertices.front());
   sortedVertices.reserve(vertices.size());
   int cursor = 0;
   for (int i = 1; i < vertices.size() - 1; ++i)
   {
      if (abs(Area2(lowRight, vertices[i], vertices[i + 1])) <= 0.0001f) // area is almost 0
      {
         if (Dist2(lowRight, vertices[i]) < Dist2(lowRight, vertices[i + 1]))
            sortedVertices.back() = &vertices[i + 1]; // if [i + 1] is more distant, then replace [cursor] with it
      }
      else
      {
         sortedVertices.push_back(&vertices[i]);
      }
   }
   if (abs(Area2(*sortedVertices[0], *sortedVertices[1], *sortedVertices.back())) > 0.0001f)
      sortedVertices.push_back(&vertices.back());

   // Initializing stack
   std::stack<Vertex*> hullStack;
   hullStack.push(sortedVertices.front());
   int i = 1;
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
   Vertex* top = hullStack.top();
   while (hullStack.size() > 1)
   {
      Vertex* v1 = hullStack.top();
      hullStack.pop();
      Vertex* v0 = hullStack.top();
      hullEdges.push_back({ *v0, *v1 });
   }
   Vertex* bottom = hullStack.top();
   hullEdges.push_back({ *top, *bottom });
}