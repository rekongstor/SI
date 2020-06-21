#include "Mesh.h"
#include "../3rd_party/OBJ-Loader/Source/OBJ_Loader.h"

Mesh::Mesh()
{
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
         {v.Normal.X, v.Normal.Y, v.Normal.Z, 0.f},
         {v.TextureCoordinate.X,v.TextureCoordinate.Y} };
   }

   //std::copy(meshLoader.LoadedIndices.begin(), meshLoader.LoadedIndices.end(), indices);
   return;
}
