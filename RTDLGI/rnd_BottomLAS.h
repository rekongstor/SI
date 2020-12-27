#pragma once
#include "rnd_IndexBuffer.h"
#include "rnd_VertexBuffer.h"

class rnd_BottomLAS : public rnd_UploadableBuffer
{
public:
   void OnInit(rnd_IndexBuffer& indexBuffer, rnd_VertexBuffer& vertexBuffer, LPCWSTR name = L"");

   void CleanUploadData() override {}
};
