#pragma once
#include "rnd_Tensor.h"

struct rnd_DmlOperator
{
   ComPtr<IDMLOperator> dmlOperator;
   ComPtr<IDMLCompiledOperator> dmlCompiledOperator;
   rnd_DynamicTensor output;

   void InitMul(rnd_Tensor& A, rnd_Tensor& B, rnd_Tensor& C, LPCWSTR name);
   void InitTanh(rnd_Tensor& A, LPCWSTR name);
};

class PassDLGI
{
public:
   rnd_DynamicTensor inputData;
   std::map<std::string, rnd_InputTensor> inputTensors;
   std::map<std::string, rnd_DmlOperator> operators;

   void OnInit();
};

