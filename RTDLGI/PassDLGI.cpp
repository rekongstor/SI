#include "PassDLGI.h"
#define TINYEXR_IMPLEMENTATION
#include "../3rd_party/tinyexr/tinyexr.h"
#include "DirectXPackedVector.h"
using namespace DirectX::PackedVector;
#include "rnd_Dx12.h"

#define DML_TARGET_VERSION_USE_LATEST
#include "../3rd_party/DirectML/Libraries/DirectMLX.h"

const char* B1 = "data\\dlgi\\B1.exr";
const char* B2b1 = "data\\dlgi\\B2b1.exr";
const char* B2b2 = "data\\dlgi\\B2b2.exr";
const char* B2b3 = "data\\dlgi\\B2b3.exr";
const char* B2w1 = "data\\dlgi\\B2w1.exr";
const char* B2w2 = "data\\dlgi\\B2w2.exr";
const char* B2w3 = "data\\dlgi\\B2w3.exr";
const char* B3b1 = "data\\dlgi\\B3b1.exr";
const char* B3b2 = "data\\dlgi\\B3b2.exr";
const char* B3b3 = "data\\dlgi\\B3b3.exr";
const char* B3w1 = "data\\dlgi\\B3w1.exr";
const char* B3w2 = "data\\dlgi\\B3w2.exr";
const char* B3w3 = "data\\dlgi\\B3w3.exr";
const char* W1 = "data\\dlgi\\W1.exr";
const char* W2b1 = "data\\dlgi\\W2b1.exr";
const char* W2b2 = "data\\dlgi\\W2b2.exr";
const char* W2b3 = "data\\dlgi\\W2b3.exr";
const char* W2w1 = "data\\dlgi\\W2w1.exr";
const char* W2w2 = "data\\dlgi\\W2w2.exr";
const char* W2w3 = "data\\dlgi\\W2w3.exr";
const char* W3b1 = "data\\dlgi\\W3b1.exr";
const char* W3b2 = "data\\dlgi\\W3b2.exr";
const char* W3b3 = "data\\dlgi\\W3b3.exr";
const char* W3w1 = "data\\dlgi\\W3w1.exr";
const char* W3w2 = "data\\dlgi\\W3w2.exr";
const char* W3w3 = "data\\dlgi\\W3w3.exr";


void PassDLGI::OnInit()
{
   const char* inputs[] = {
      B1, B2b1, B2b2, B2b3, B2w1, B2w2, B2w3, B3b1, B3b2, B3b3, B3w1, B3w2, B3w3,
      W1, W2b1, W2b2, W2b3, W2w1, W2w2, W2w3, W3b1, W3b2, W3b3, W3w1, W3w2, W3w3,
   };
   float* out; // width * height * RGBA
   int width;
   int height;
   const char* err = NULL; // or nullptr in C++11

   std::vector<char> data;
   for (auto& input : inputs)
   {
      int ret = LoadEXR(&out, &width, &height, input, &err);
      bool odd = (width * height) % 4 != 0;

      data.resize(((width * height) / 4 + (odd ? 1 : 0)) * sizeof(XMHALF2));
      XMHALF2 tmp;
      for (int i = 0; i < width * height / 4; ++i)
      {
         XMStoreHalf2(&tmp, {out[i * 4], out[(i + 1) * 4]});
         XMHALF2* dataPtr = (XMHALF2*)(data.data());
         dataPtr[i] = tmp;
      }
      if (odd)
      {
         XMStoreHalf2(&tmp, {out[width * height / 4 + 1], 0});
         XMHALF2* dataPtr = (XMHALF2*)(data.data());
         dataPtr[width * height / 4] = tmp;
      }

      auto& tensor = inputTensors[input];

      tensor.tensorSizes[2] = width;
      tensor.tensorSizes[3] = height;

      DML_BUFFER_TENSOR_DESC& dmlBufferTensorDesc = tensor.tensorDesc;
      dmlBufferTensorDesc.DataType = DML_TENSOR_DATA_TYPE_FLOAT16;
      dmlBufferTensorDesc.Flags = DML_TENSOR_FLAG_NONE;
      dmlBufferTensorDesc.DimensionCount = ARRAYSIZE(tensor.tensorSizes);
      dmlBufferTensorDesc.Sizes = tensor.tensorSizes;
      dmlBufferTensorDesc.Strides = nullptr;
      dmlBufferTensorDesc.TotalTensorSizeInBytes = DMLCalcBufferTensorSize(
         dmlBufferTensorDesc.DataType,
         dmlBufferTensorDesc.DimensionCount,
         dmlBufferTensorDesc.Sizes,
         dmlBufferTensorDesc.Strides);

      tensor.AssignData(data);
   }

   // Operators
   {
      operators["W1B1"].InitMul(inputData, inputTensors[W1], inputTensors[B1], L"W1B1");
      operators["N1"].InitTanh(operators["W1B1"].output, L"N1");

      operators["W2B2w1"].InitMul(operators["N1"].output, inputTensors[W2w1], inputTensors[B2w1], L"W2B2w1");
      operators["N2w1"].InitTanh(operators["W2B2w1"].output, L"N2w1");
      operators["N3w1"].InitMul(operators["N2w1"].output, inputTensors[W3w1], inputTensors[B3w1], L"N3w1");

      operators["W2B2w2"].InitMul(operators["N1"].output, inputTensors[W2w2], inputTensors[B2w2], L"W2B2w2");
      operators["N2w2"].InitTanh(operators["W2B2w2"].output, L"N2w2");
      operators["N3w2"].InitMul(operators["N2w2"].output, inputTensors[W3w2], inputTensors[B3w2], L"N3w2");

      operators["W2B2w3"].InitMul(operators["N1"].output, inputTensors[W2w3], inputTensors[B2w3], L"W2B2w3");
      operators["N2w3"].InitTanh(operators["W2B2w3"].output, L"N2w3");
      operators["N3w3"].InitMul(operators["N2w3"].output, inputTensors[W3w3], inputTensors[B3w3], L"N3w3");

      operators["W2B2b1"].InitMul(operators["N1"].output, inputTensors[W2b1], inputTensors[B2b1], L"W2B2b1");
      operators["N2b1"].InitTanh(operators["W2B2b1"].output, L"N2b1");
      operators["N3b1"].InitMul(operators["N2b1"].output, inputTensors[W3b1], inputTensors[B3b1], L"N3b1");

      operators["W2B2b2"].InitMul(operators["N1"].output, inputTensors[W2b2], inputTensors[B2b2], L"W2B2b2");
      operators["N2b2"].InitTanh(operators["W2B2b2"].output, L"N2b2");
      operators["N3b2"].InitMul(operators["N2b2"].output, inputTensors[W3b2], inputTensors[B3b2], L"N3b2");

      operators["W2B2b3"].InitMul(operators["N1"].output, inputTensors[W2b3], inputTensors[B2b3], L"W2B2b3");
      operators["N2b3"].InitTanh(operators["W2B2b3"].output, L"N2b3");
      operators["N3b3"].InitMul(operators["N2b3"].output, inputTensors[W3b3], inputTensors[B3b3], L"N3b3");
   }
}

void rnd_DmlOperator::InitMul(rnd_Tensor& A, rnd_Tensor& B, rnd_Tensor& C, LPCWSTR name)
{
   DML_TENSOR_DESC dmlTensorDescA{ DML_TENSOR_TYPE_BUFFER, &A.tensorDesc };
   DML_TENSOR_DESC dmlTensorDescB{ DML_TENSOR_TYPE_BUFFER, &B.tensorDesc };
   DML_TENSOR_DESC dmlTensorDescC{ DML_TENSOR_TYPE_BUFFER, &C.tensorDesc };

   output.tensorSizes[2] = A.tensorDesc.Sizes[2];
   output.tensorSizes[3] = B.tensorDesc.Sizes[3];

   DML_BUFFER_TENSOR_DESC& dmlBufferTensorDesc = output.tensorDesc;
   dmlBufferTensorDesc.DataType = DML_TENSOR_DATA_TYPE_FLOAT16;
   dmlBufferTensorDesc.Flags = DML_TENSOR_FLAG_NONE;
   dmlBufferTensorDesc.DimensionCount = ARRAYSIZE(output.tensorSizes);
   dmlBufferTensorDesc.Sizes = output.tensorSizes;
   dmlBufferTensorDesc.Strides = nullptr;
   dmlBufferTensorDesc.TotalTensorSizeInBytes = DMLCalcBufferTensorSize(
      dmlBufferTensorDesc.DataType,
      dmlBufferTensorDesc.DimensionCount,
      dmlBufferTensorDesc.Sizes,
      dmlBufferTensorDesc.Strides);

   DML_TENSOR_DESC dmlTensorDescOut{ DML_TENSOR_TYPE_BUFFER, &output.tensorDesc };

   DML_GEMM_OPERATOR_DESC operatorDesc{};
   operatorDesc.ATensor = &dmlTensorDescA;
   operatorDesc.BTensor = &dmlTensorDescB;
   operatorDesc.CTensor = &dmlTensorDescC;
   operatorDesc.OutputTensor = &dmlTensorDescOut;
   operatorDesc.TransA = DML_MATRIX_TRANSFORM_NONE;
   operatorDesc.TransB = DML_MATRIX_TRANSFORM_NONE;
   operatorDesc.Alpha = 1.f;
   operatorDesc.Beta = 1.f;
   operatorDesc.FusedActivation = nullptr;

   DML_OPERATOR_DESC dmlOperatorDesc{ DML_OPERATOR_GEMM, &operatorDesc };

   ThrowIfFailed(renderer->DmlDevice()->CreateOperator(&dmlOperatorDesc, IID_PPV_ARGS(&dmlOperator)));
   ThrowIfFailed(renderer->DmlDevice()->CompileOperator(dmlOperator.Get(), DML_EXECUTION_FLAG_ALLOW_HALF_PRECISION_COMPUTATION, IID_PPV_ARGS(&dmlCompiledOperator)));

   dmlOperator->SetName(FormatWStr(L"%s [Op]", name));
   dmlCompiledOperator->SetName(FormatWStr(L"%s [COp]", name));
   
   output.OnInit(name);
}

void rnd_DmlOperator::InitTanh(rnd_Tensor& A, LPCWSTR name)
{
   DML_TENSOR_DESC dmlTensorDescA{ DML_TENSOR_TYPE_BUFFER, &A.tensorDesc };

   output.tensorSizes[2] = A.tensorDesc.Sizes[2];
   output.tensorSizes[3] = A.tensorDesc.Sizes[3];

   DML_BUFFER_TENSOR_DESC& dmlBufferTensorDesc = output.tensorDesc;
   dmlBufferTensorDesc.DataType = DML_TENSOR_DATA_TYPE_FLOAT16;
   dmlBufferTensorDesc.Flags = DML_TENSOR_FLAG_NONE;
   dmlBufferTensorDesc.DimensionCount = ARRAYSIZE(output.tensorSizes);
   dmlBufferTensorDesc.Sizes = output.tensorSizes;
   dmlBufferTensorDesc.Strides = nullptr;
   dmlBufferTensorDesc.TotalTensorSizeInBytes = DMLCalcBufferTensorSize(
      dmlBufferTensorDesc.DataType,
      dmlBufferTensorDesc.DimensionCount,
      dmlBufferTensorDesc.Sizes,
      dmlBufferTensorDesc.Strides);

   DML_TENSOR_DESC dmlTensorDescOut{ DML_TENSOR_TYPE_BUFFER, &output.tensorDesc };

   DML_ELEMENT_WISE_TANH_OPERATOR_DESC operatorDesc{};
   operatorDesc.InputTensor = &dmlTensorDescA;
   operatorDesc.OutputTensor = &dmlTensorDescOut;
   operatorDesc.ScaleBias = nullptr;

   DML_OPERATOR_DESC dmlOperatorDesc{ DML_OPERATOR_ELEMENT_WISE_TANH, &operatorDesc };

   ThrowIfFailed(renderer->DmlDevice()->CreateOperator(&dmlOperatorDesc, IID_PPV_ARGS(&dmlOperator)));

   ThrowIfFailed(renderer->DmlDevice()->CompileOperator(dmlOperator.Get(), DML_EXECUTION_FLAG_ALLOW_HALF_PRECISION_COMPUTATION, IID_PPV_ARGS(&dmlCompiledOperator)));

   dmlOperator->SetName(FormatWStr(L"%s [Op]", name));
   dmlCompiledOperator->SetName(FormatWStr(L"%s [COp]", name));

   output.OnInit(name);
}
