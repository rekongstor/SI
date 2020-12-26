#pragma once
#include <iosfwd>
#include <vector>


#include "HlslCompat.h"
#include "rnd_IndexBuffer.h"
#include "rnd_VertexBuffer.h"

class rnd_Mesh
{
public:
   rnd_VertexBuffer vertexBuffer;
   rnd_IndexBuffer indexBuffer;

   void OnInit(std::vector<char>& vertices, std::vector<char>& indices, LPCWSTR name = L"");
};

