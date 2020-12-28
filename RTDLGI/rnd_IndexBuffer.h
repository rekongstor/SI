#pragma once
#include "rnd_UploadableBuffer.h"

class rnd_IndexBuffer : public rnd_UploadableBuffer, public Buffer1D
{
public:
   void OnInit(std::vector<char>& data, int sizeOfElement, LPCWSTR name = L"");
   void CreateSrv();

   D3D12_INDEX_BUFFER_VIEW indexBufferView;
   DescHandlePair srvHandle;

   virtual ~rnd_IndexBuffer() = default;
};