#pragma once
#include "rnd_IndexBuffer.h"
#include "rnd_VertexBuffer.h"

class rnd_BottomLAS : public rnd_UploadableBuffer
{
   ID3D12Resource* scratchResource;
public:
   void OnInit(rnd_IndexBuffer& indexBuffer, rnd_VertexBuffer& vertexBuffer);

   void CleanUploadData() override {}
};
