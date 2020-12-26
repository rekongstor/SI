#pragma once
#include "rnd_UploadableBuffer.h"

class rnd_VertexBuffer : public rnd_UploadableBuffer
{
public:
   void OnInit(std::vector<char>& data, int sizeOfElement, LPCWSTR name = L"");
   void CreateSrv();

   D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
   DescHandlePair srvHandle;

   virtual ~rnd_VertexBuffer() = default;
};
