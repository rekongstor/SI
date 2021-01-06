#pragma once
#include <DirectML.h>

#include "rnd_UploadableBuffer.h"

struct rnd_Tensor
{
   UINT tensorSizes[4]{ 1,1,1,1 };
   DML_BUFFER_TENSOR_DESC tensorDesc;
};

class rnd_DynamicTensor : public rnd_Tensor, public rnd_Buffer
{
public:
   void OnInit(LPCWSTR name);
};

class rnd_InputTensor : public rnd_Tensor, public rnd_UploadableBuffer
{
public:
   void AssignData(std::vector<char>& data);
   void OnInit(LPCWSTR name);
};

