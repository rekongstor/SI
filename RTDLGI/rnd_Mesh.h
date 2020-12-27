#pragma once
#include "rnd_IndexBuffer.h"
#include "rnd_VertexBuffer.h"
#include "rnd_BottomLAS.h"

class rnd_Mesh
{
public:
   rnd_VertexBuffer vertexBuffer;
   rnd_IndexBuffer indexBuffer;
   rnd_BottomLAS bottomLas;

   void OnInit(std::vector<char>& vertices, std::vector<char>& indices, LPCWSTR name = L"");
};

