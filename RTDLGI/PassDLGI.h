#pragma once
#include "rnd_Tensor.h"

struct rnd_DmlOperator
{
   ComPtr<IDMLOperator> dmlOperator;
   ComPtr<IDMLCompiledOperator> dmlCompiledOperator;
   rnd_DynamicTensor output;
   std::wstring name;
   DML_BINDING_PROPERTIES bindingProperties;
   DML_BUFFER_BINDING persBufferBinding;
   DML_BINDING_TABLE_DESC dmlBindingTableDesc;

   std::vector<DML_BUFFER_BINDING> bufferBindings;
   std::vector<DML_BINDING_DESC> bindingDescs;

   std::vector<rnd_Tensor*> tensors;

   void InitMul(rnd_Tensor& A, rnd_Tensor& B, rnd_Tensor& C, LPCWSTR name);
   void InitTanh(rnd_Tensor& A, LPCWSTR name);

   void OnInit();
   void Execute();
};

class PassDLGI
{
public:
   rnd_DynamicTensor inputData;
   std::map<std::string, rnd_InputTensor> inputTensors;
   std::map<std::string, rnd_DmlOperator> operators;

   ComPtr<IDMLOperatorInitializer> dmlOperatorInitializer;
   DML_BINDING_PROPERTIES bindingProperties;

   ComPtr<IDMLBindingTable> dmlBindingTable;

   DML_BUFFER_BINDING tmpBufferBinding;
   rnd_DynamicTensor temporaryResource;

   ComPtr<IDMLCommandRecorder> dmlCommandRecorder;
   ComPtr<ID3D12DescriptorHeap> dmlDescriptorHeap;
   void OnInit();
   void Execute();
};

