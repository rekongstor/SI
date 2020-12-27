#pragma once
#include "rnd_UploadableBuffer.h"

class rnd_Scene;

class rnd_TopLAS : public rnd_UploadableBuffer
{
   ComPtr<ID3D12Resource> instanceData;
public:
   void OnInit(rnd_Scene* scene);

   void CleanUploadData() override {}
};

