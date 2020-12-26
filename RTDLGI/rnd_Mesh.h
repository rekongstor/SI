#pragma once
#include <iosfwd>
#include <vector>


#include "HlslCompat.h"
#include "rnd_IndexBuffer.h"
#include "rnd_VertexBuffer.h"

class rnd_Mesh
{
   rnd_VertexBuffer vertexBuffer;
   rnd_IndexBuffer indexBuffer;

   void OnInit(const std::vector<Vertex>& vertices, const std::vector<Index>& indices, LPCWSTR name = L"");
};

