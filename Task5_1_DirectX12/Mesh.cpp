#include "Mesh.h"
#include "../3rd_party/OBJ-Loader/Source/OBJ_Loader.h"

Mesh::Mesh()
{
   vertices.assign({
      {{0.5f, 0.5f, 0.5f, 1.f},     {1.0f, 0.05f, 0.05f, 1.0f}, {0.5773f, 0.5773f, 0.5773f, 0.0f}},
      {{0.5f, 0.5f, -0.5f, 1.f},    {1.0f, 0.05f, 0.05f, 1.0f}, {0.5773f, 0.5773f, -0.5773f, 0.0f}},
      {{0.5f, -0.5f, 0.5f, 1.f},    {1.0f, 0.05f, 0.05f, 1.0f}, {0.5773f, -0.5773f, 0.5773f, 0.0f}},
      {{0.5f, -0.5f, -0.5f, 1.f},   {1.0f, 0.05f, 0.05f, 1.0f}, {0.5773f, -0.5773f, -0.5773f, 0.0f}},
      {{-0.5f, 0.5f, 0.5f, 1.f},    {1.0f, 0.05f, 0.05f, 1.0f}, {-0.5773f, 0.5773f, 0.5773f, 0.0f}},
      {{-0.5f, 0.5f, -0.5f, 1.f},   {1.0f, 0.05f, 0.05f, 1.0f}, {-0.5773f, 0.5773f, -0.5773f, 0.0f}},
      {{-0.5f, -0.5f, 0.5f, 1.f},   {1.0f, 0.05f, 0.05f, 1.0f}, {-0.5773f, -0.5773f, 0.5773f, 0.0f}},
      {{-0.5f, -0.5f, -0.5f, 1.f},  {1.0f, 0.05f, 0.05f, 1.0f}, {-0.5773f, -0.5773f, -0.5773f, 0.0f}},
   });

   indices.assign({
      0, 2, 1,
      2, 3, 1,

      6, 4, 5,
      7, 6, 5,

      7, 5, 1,
      3, 7, 1,

      2, 0, 4,
      6, 2, 4,

      4, 0, 1,
      5, 4, 1,

      6, 3, 2,
      7, 3, 6,
   });
}

Mesh::Mesh(const char* filename)
{
   objl::Loader meshLoader;
   meshLoader.LoadFile(filename);

   indices.resize(meshLoader.LoadedIndices.size());
   vertices.resize(meshLoader.LoadedVertices.size());

   for (size_t i = 0; i < meshLoader.LoadedIndices.size(); ++i)
      indices[i] = meshLoader.LoadedIndices[i];

   for (size_t i = 0; i < meshLoader.LoadedVertices.size(); ++i)
   {
      auto&& v = meshLoader.LoadedVertices[i];
      vertices[i] = {
         {v.Position.X, v.Position.Y, v.Position.Z, 1.f},
         {},
         {v.Normal.X, v.Normal.Y, v.Normal.Z, 0.f},
         {v.TextureCoordinate.X,v.TextureCoordinate.Y} };
   }

   //std::copy(meshLoader.LoadedIndices.begin(), meshLoader.LoadedIndices.end(), indices);
   return;
}
