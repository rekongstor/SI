#pragma once
#include <DirectML.h>

#include "rnd_UploadableBuffer.h"

struct rnd_Tensor : public Buffer1D
{
   UINT tensorSizes[4]{ 1,1,1,1 };
   DML_BUFFER_TENSOR_DESC tensorDesc;
   virtual rnd_Buffer* Buffer() = 0;

   DescHandlePair uavHandle;
   void CreateUav();
};

class rnd_DynamicTensor : public rnd_Tensor, public rnd_Buffer
{
public:
   void OnInit(UINT64 size, LPCWSTR name = L"");
   void RecalcSizes();
   rnd_Buffer* Buffer() override { return this; }
};

class rnd_InputTensor : public rnd_Tensor, public rnd_UploadableBuffer
{
public:
   void AssignData(std::vector<char>& data);
   void OnInit(UINT64 size, LPCWSTR name);
   rnd_Buffer* Buffer() override { return this; }
};

