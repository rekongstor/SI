#pragma once
class rnd_IndexBuffer : public D3DBuffer
{
public:
   void OnInit(void* srcData, UINT64 sizeInBytes, LPCWSTR name = L"");
   void CreateSrv();

   D3D12_INDEX_BUFFER_VIEW indexBufferView;
   DescHandlePair srvHandle;
};