#pragma once
#include "D3DBuffer.h"

class rnd_VertexBuffer : public D3DBuffer
{
public:
   void OnInit(void* srcData, UINT64 sizeInBytes, int sizeOfElement, LPCWSTR name = L"");
   void CreateSrv();

   D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
   DescHandlePair srvHandle;
};