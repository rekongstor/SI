#pragma once
#include "rnd_UploadableBuffer.h"

class rnd_Scene;

class rnd_TopLAS : public rnd_UploadableBuffer
{
   ComPtr<ID3D12Resource> altBuffer;
   ComPtr<ID3D12Resource> instanceData;
   ComPtr<ID3D12Resource> scratchResource;
   D3D12_RAYTRACING_INSTANCE_DESC* mappedData;
public:
   void OnInit(rnd_Scene* scene, LPCWSTR name = L"");
   void OnUpdate(rnd_Scene* scene);

   void CleanUploadData() override {}
};

