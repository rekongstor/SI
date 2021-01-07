#include "PassDLGI.h"
#define TINYEXR_IMPLEMENTATION
#include "../3rd_party/tinyexr/tinyexr.h"
#include "DirectXPackedVector.h"
using namespace DirectX::PackedVector;
#include "rnd_Dx12.h"

#define DML_TARGET_VERSION_USE_LATEST
#include "../3rd_party/DirectML/Libraries/DirectMLX.h"

#define NEURONS_COUNT_GI 64

#pragma region DLGI NAMES
namespace dlgiFiles
{
   static const char* B1 = "data\\dlgi\\B1.exr";
   static const char* B2b1 = "data\\dlgi\\B2b1.exr";
   static const char* B2b2 = "data\\dlgi\\B2b2.exr";
   static const char* B2b3 = "data\\dlgi\\B2b3.exr";
   static const char* B2w1 = "data\\dlgi\\B2w1.exr";
   static const char* B2w2 = "data\\dlgi\\B2w2.exr";
   static const char* B2w3 = "data\\dlgi\\B2w3.exr";
   static const char* B3b1 = "data\\dlgi\\B3b1.exr";
   static const char* B3b2 = "data\\dlgi\\B3b2.exr";
   static const char* B3b3 = "data\\dlgi\\B3b3.exr";
   static const char* B3w1 = "data\\dlgi\\B3w1.exr";
   static const char* B3w2 = "data\\dlgi\\B3w2.exr";
   static const char* B3w3 = "data\\dlgi\\B3w3.exr";
   static const char* W1 = "data\\dlgi\\W1.exr";
   static const char* W2b1 = "data\\dlgi\\W2b1.exr";
   static const char* W2b2 = "data\\dlgi\\W2b2.exr";
   static const char* W2b3 = "data\\dlgi\\W2b3.exr";
   static const char* W2w1 = "data\\dlgi\\W2w1.exr";
   static const char* W2w2 = "data\\dlgi\\W2w2.exr";
   static const char* W2w3 = "data\\dlgi\\W2w3.exr";
   static const char* W3b1 = "data\\dlgi\\W3b1.exr";
   static const char* W3b2 = "data\\dlgi\\W3b2.exr";
   static const char* W3b3 = "data\\dlgi\\W3b3.exr";
   static const char* W3w1 = "data\\dlgi\\W3w1.exr";
   static const char* W3w2 = "data\\dlgi\\W3w2.exr";
   static const char* W3w3 = "data\\dlgi\\W3w3.exr";
}

namespace dlgiOps
{
   const char* W1B1 = "W1B1";
   const char* N1 = "N1";
   const char* W2B2w1 = "W2B2w1";
   const char* N2w1 = "N2w1";
   const char* N3w1 = "N3w1";
   const char* W2B2w2 = "W2B2w2";
   const char* N2w2 = "N2w2";
   const char* N3w2 = "N3w2";
   const char* W2B2w3 = "W2B2w3";
   const char* N2w3 = "N2w3";
   const char* N3w3 = "N3w3";
   const char* W2B2b1 = "W2B2b1";
   const char* N2b1 = "N2b1";
   const char* N3b1 = "N3b1";
   const char* W2B2b2 = "W2B2b2";
   const char* N2b2 = "N2b2";
   const char* N3b2 = "N3b2";
   const char* W2B2b3 = "W2B2b3";
   const char* N2b3 = "N2b3";
   const char* N3b3 = "N3b3";

   const char* w1b1 = "w1b1";
   const char* n1 = "n1";
   const char* w2b2 = "w2b2";
   const char* n2 = "n2";
   const char* w3b3 = "w3b3";
   const char* n3 = "n3";
}
#pragma endregion 

using namespace dlgiFiles;
using namespace dlgiOps;

void PassDLGI::OnInit()
{
   const char* inputs[] = {
      W1, W2b1, W2b2, W2b3, W2w1, W2w2, W2w3, W3b1, W3b2, W3b3, W3w1, W3w2, W3w3,
      B1, B2b1, B2b2, B2b3, B2w1, B2w2, B2w3, B3b1, B3b2, B3b3, B3w1, B3w2, B3w3,
   };
   float* out; // width * height * RGBA
   int width;
   int height;
   const char* err = nullptr; // or nullptr in C++11

   for (auto& input : inputs)
   {
      int ret = LoadEXR(&out, &width, &height, input, &err);
      bool odd = (width * height) % 2 != 0;

      std::vector<float> imtired(width * height);
      for (int i = 0; i < width * height; ++i)
         imtired[i] = out[i * 4];

      auto GetCell = [&](int x, int y)
      {
         return imtired.data()[(x + y * width)];
      };


      std::vector<char> data;
      data.resize(((width * height / 2) + (odd ? 1 : 0)) * sizeof(XMHALF2));
      XMHALF2 tmp;
      for (int i = 0; i < width; ++i) {
         for (int j = 0; j < height; ++j) {
            if ((i * height + j) % 2 == 1) // skip every odd value
               continue;

            float f1 = GetCell(i, j);
            float f2 = GetCell(i, j + 1);
            XMStoreHalf2(&tmp, { f1, f2 });
            XMHALF2* dataPtr = (XMHALF2*)(data.data());
            dataPtr[(i * height + j) / 2] = tmp;
         }
      }
      //for (int i = 0; i < width * height / 2; ++i)
      //{
      //   float f1 = out[i * 2 * 4];
      //   float f2 = out[(i * 2 + 1) * 4];
      //   XMStoreHalf2(&tmp, {f1, f2});
      //   XMHALF2* dataPtr = (XMHALF2*)(data.data());
      //   dataPtr[i] = tmp;
      //}
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

   // Input RT data
   {
      auto& tensor = inputRtData;

      tensor.tensorSizes[2] = 1; // Dist
      tensor.tensorSizes[3] = RAYS_PER_AXIS * RAYS_PER_AXIS * RAYS_PER_AXIS; // RA

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

      tensor.OnInit(dmlBufferTensorDesc.TotalTensorSizeInBytes, L"RT_Tensor");
   }
   // Input Pos data
   {
      auto& tensor = inputPosData;

      tensor.tensorSizes[0] = GI_RESOLUTION * GI_RESOLUTION * GI_RESOLUTION; // XYZ
      tensor.tensorSizes[3] = 3;

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

      tensor.OnInit(dmlBufferTensorDesc.TotalTensorSizeInBytes, L"Pos_Tensor");
   }

   // Operators
   {
      // I think it's easier to preInit all
      operators[W1B1];
      operators[N1];

      operators[W2B2w1];
      operators[N2w1];
      operators[N3w1];

      operators[W2B2w2];
      operators[N2w2];
      operators[N3w2];

      operators[W2B2w3];
      operators[N2w3];
      operators[N3w3];

      operators[W2B2b1];
      operators[N2b1];
      operators[N3b1];

      operators[W2B2b2];
      operators[N2b2];
      operators[N3b2];

      operators[W2B2b3];
      operators[N2b3];
      operators[N3b3];

      //operators[w1b1];
      //operators[n1];
      //operators[w2b2];
      //operators[n2];
      //operators[w3b3];
      //operators[n3];

      operators[W1B1].InitMul(inputRtData, inputTensors[W1], inputTensors[B1], L"W1B1");
      operators[N1].InitTanh(operators[W1B1].output, L"N1");

      operators[W2B2w1].InitMul(operators[N1].output, inputTensors[W2w1], inputTensors[B2w1], L"W2B2w1");
      operators[N2w1].InitTanh(operators[W2B2w1].output, L"N2w1");
      operators[N3w1].InitMul(operators[N2w1].output, inputTensors[W3w1], inputTensors[B3w1], L"N3w1");

      operators[W2B2w2].InitMul(operators[N1].output, inputTensors[W2w2], inputTensors[B2w2], L"W2B2w2");
      operators[N2w2].InitTanh(operators[W2B2w2].output, L"N2w2");
      operators[N3w2].InitMul(operators[N2w2].output, inputTensors[W3w2], inputTensors[B3w2], L"N3w2");

      operators[W2B2w3].InitMul(operators[N1].output, inputTensors[W2w3], inputTensors[B2w3], L"W2B2w3");
      operators[N2w3].InitTanh(operators[W2B2w3].output, L"N2w3");
      operators[N3w3].InitMul(operators[N2w3].output, inputTensors[W3w3], inputTensors[B3w3], L"N3w3");

      operators[W2B2b1].InitMul(operators[N1].output, inputTensors[W2b1], inputTensors[B2b1], L"W2B2b1");
      operators[N2b1].InitTanh(operators[W2B2b1].output, L"N2b1");
      operators[N3b1].InitMul(operators[N2b1].output, inputTensors[W3b1], inputTensors[B3b1], L"N3b1");

      operators[W2B2b2].InitMul(operators[N1].output, inputTensors[W2b2], inputTensors[B2b2], L"W2B2b2");
      operators[N2b2].InitTanh(operators[W2B2b2].output, L"N2b2");
      operators[N3b2].InitMul(operators[N2b2].output, inputTensors[W3b2], inputTensors[B3b2], L"N3b2");

      operators[W2B2b3].InitMul(operators[N1].output, inputTensors[W2b3], inputTensors[B2b3], L"W2B2b3");
      operators[N2b3].InitTanh(operators[W2B2b3].output, L"N2b3");
      operators[N3b3].InitMul(operators[N2b3].output, inputTensors[W3b3], inputTensors[B3b3], L"N3b3");

      //w1 = operators[N3w1].output;
      //b1 = operators[N3b1].output;
      //w2 = operators[N3w2].output;
      //b2 = operators[N3b2].output;
      //w3 = operators[N3w3].output;
      //b3 = operators[N3b3].output;

      //w1.tensorSizes[2] = 3;
      //w1.tensorSizes[3] = NEURONS_COUNT_GI;
      //b1.tensorSizes[2] = 1;
      //b2.tensorSizes[3] = NEURONS_COUNT_GI;

      //w2.tensorSizes[2] = NEURONS_COUNT_GI;
      //w2.tensorSizes[3] = NEURONS_COUNT_GI;
      //b2.tensorSizes[2] = 1;
      //b2.tensorSizes[3] = NEURONS_COUNT_GI;

      //w3.tensorSizes[2] = NEURONS_COUNT_GI;
      //w3.tensorSizes[3] = 1;
      //b3.tensorSizes[1] = 1;
      //b3.tensorSizes[1] = 1;

      //w1.tensorSizes[0] = GI_RESOLUTION * GI_RESOLUTION * GI_RESOLUTION;
      //b1.tensorSizes[0] = GI_RESOLUTION * GI_RESOLUTION * GI_RESOLUTION;
      //w2.tensorSizes[0] = GI_RESOLUTION * GI_RESOLUTION * GI_RESOLUTION;
      //b2.tensorSizes[0] = GI_RESOLUTION * GI_RESOLUTION * GI_RESOLUTION;
      //w3.tensorSizes[0] = GI_RESOLUTION * GI_RESOLUTION * GI_RESOLUTION;
      //b3.tensorSizes[0] = GI_RESOLUTION * GI_RESOLUTION * GI_RESOLUTION;


      //w1.tensorDesc.Sizes = w1.tensorSizes;
      //b1.tensorDesc.Sizes = b1.tensorSizes;
      //w2.tensorDesc.Sizes = w2.tensorSizes;
      //b2.tensorDesc.Sizes = b2.tensorSizes;
      //w3.tensorDesc.Sizes = w3.tensorSizes;
      //b3.tensorDesc.Sizes = b3.tensorSizes;

      //w1.RecalcSizes();
      //b1.RecalcSizes();
      //w2.RecalcSizes();
      //b2.RecalcSizes();
      //w3.RecalcSizes();
      //b3.RecalcSizes();

      //w1.OnInit(w1.tensorDesc.TotalTensorSizeInBytes);
      //b1.OnInit(b1.tensorDesc.TotalTensorSizeInBytes);
      //w2.OnInit(w2.tensorDesc.TotalTensorSizeInBytes);
      //b2.OnInit(b2.tensorDesc.TotalTensorSizeInBytes);
      //w3.OnInit(w3.tensorDesc.TotalTensorSizeInBytes);
      //b3.OnInit(b3.tensorDesc.TotalTensorSizeInBytes);

      //operators[w1b1].InitMul(inputPosData, w1, b1, L"w1b1");
      //operators[n1].InitTanh(operators[w1b1].output, L"n1");
      //operators[w2b2].InitMul(operators[n1].output, w2, b2, L"w2b2");
      //operators[n2].InitTanh(operators[w2b2].output, L"n2");
      //operators[w3b3].InitMul(operators[n2].output, w3, b3, L"w3b3");
      //operators[n3].InitTanh(operators[w3b3].output, L"n3 - output GI");
   }

   std::vector<IDMLCompiledOperator*> compiledOperators;
   for (auto& op : operators)
      compiledOperators.push_back(op.second.dmlCompiledOperator.Get());

   ThrowIfFailed(renderer->DmlDevice()->CreateOperatorInitializer(compiledOperators.size(), compiledOperators.data(), IID_PPV_ARGS(&dmlOperatorInitializer)));

   bindingProperties = dmlOperatorInitializer->GetBindingProperties();
   UINT descriptorCount = bindingProperties.RequiredDescriptorCount;
   UINT64 tmpResSize = bindingProperties.TemporaryResourceSize;
   for (auto& op : operators)
   {
      descriptorCount += op.second.bindingProperties.RequiredDescriptorCount;
      tmpResSize = std::max(tmpResSize, op.second.bindingProperties.TemporaryResourceSize);
   }


   D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
   descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
   descriptorHeapDesc.NumDescriptors = descriptorCount;
   descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
   renderer->Device()->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&dmlDescriptorHeap));
   dmlDescriptorHeap->SetName(L"DXML DescHeap");
   auto descIncrSize = renderer->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

   renderer->CommandList()->SetDescriptorHeaps(1, dmlDescriptorHeap.GetAddressOf());

   DML_BINDING_TABLE_DESC dmlBindingTableDesc;
   dmlBindingTableDesc.Dispatchable = dmlOperatorInitializer.Get();
   dmlBindingTableDesc.CPUDescriptorHandle = dmlDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
   dmlBindingTableDesc.GPUDescriptorHandle = dmlDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
   dmlBindingTableDesc.SizeInDescriptors = descriptorCount;
   ThrowIfFailed(renderer->DmlDevice()->CreateBindingTable(&dmlBindingTableDesc, IID_PPV_ARGS(&dmlBindingTable)));

   if (tmpResSize)
   temporaryResource.OnInit(tmpResSize, L"Temporary DML resource");
   std::vector<DML_BINDING_DESC> bindingDescs;
   DescHandlePair tmpPair;
   tmpPair.first = dmlDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
   tmpPair.second = dmlDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
   for (auto& op : operators)
   {
      auto count = op.second.bindingProperties.RequiredDescriptorCount;
      op.second.dmlBindingTableDesc.Dispatchable = op.second.dmlCompiledOperator.Get();
      op.second.dmlBindingTableDesc.CPUDescriptorHandle = tmpPair.first.Offset(count, descIncrSize);
      op.second.dmlBindingTableDesc.GPUDescriptorHandle = tmpPair.second.Offset(count, descIncrSize);
      op.second.dmlBindingTableDesc.SizeInDescriptors = count;
      op.second.OnInit();
      bindingDescs.push_back({ DML_BINDING_TYPE_BUFFER, &op.second.persBufferBinding });
   }
   dmlBindingTable->BindOutputs(bindingDescs.size(), bindingDescs.data());

   renderer->DmlDevice()->CreateCommandRecorder(IID_PPV_ARGS(&dmlCommandRecorder));

   dmlCommandRecorder->RecordDispatch(renderer->CommandList(), dmlOperatorInitializer.Get(), dmlBindingTable.Get());

   tmpBufferBinding = { temporaryResource.buffer.Get(), 0, bindingProperties.TemporaryResourceSize };

   int k = 0;

   for (auto& inpTens : inputTensors)
   {
      ++k;
      inpTens.second.OnInit(inpTens.second.tensorDesc.TotalTensorSizeInBytes, WStrFromStr(inpTens.first.c_str()).c_str());
   }

   renderer->ResolveUploadBuffer();

   renderer->ExecuteCommandList();
   renderer->WaitForGpu();
}

void PassDLGI::Execute()
{
   renderer->CommandList()->SetDescriptorHeaps(1, dmlDescriptorHeap.GetAddressOf());
   auto execute = [&](rnd_DmlOperator& op)
   {
      ThrowIfFailed(dmlBindingTable->Reset(&op.dmlBindingTableDesc));


      if (tmpBufferBinding.SizeInBytes > 0){
         DML_BINDING_DESC bindingDesc{ DML_BINDING_TYPE_BUFFER, &tmpBufferBinding };
         dmlBindingTable->BindTemporaryResource(&bindingDesc);
      }
      if (op.persBufferBinding.SizeInBytes > 0){
         DML_BINDING_DESC bindingDesc{ DML_BINDING_TYPE_BUFFER, &op.persBufferBinding };
         dmlBindingTable->BindPersistentResource(&bindingDesc);
      }
      {
         std::vector<DML_BUFFER_BINDING> buffers;
         std::vector<DML_BINDING_DESC> bindingDescs;

         for (auto& t : op.tensors)
         {
            buffers.push_back({ t->Buffer()->buffer.Get(), 0, t->width });
         }
         for (auto& d : buffers)
         {
            bindingDescs.push_back({ DML_BINDING_TYPE_BUFFER, &d });
         }
         dmlBindingTable->BindInputs(bindingDescs.size(), bindingDescs.data());
      }
      {
         DML_BUFFER_BINDING bufferBinding{ op.output.Buffer()->buffer.Get(), 0, op.output.width };
         DML_BINDING_DESC bindingDesc{ DML_BINDING_TYPE_BUFFER, &bufferBinding };
         dmlBindingTable->BindOutputs(1, &bindingDesc);
      }

      dmlCommandRecorder->RecordDispatch(renderer->CommandList(), op.dmlCompiledOperator.Get(), dmlBindingTable.Get());
   };

   execute(operators[dlgiOps::W1B1]);
   execute(operators[dlgiOps::N1]);

   execute(operators[dlgiOps::W2B2w1]);
   execute(operators[dlgiOps::N2w1]);
   execute(operators[dlgiOps::N3w1]);

   execute(operators[dlgiOps::W2B2b1]);
   execute(operators[dlgiOps::N2b1]);
   execute(operators[dlgiOps::N3b1]);

   execute(operators[dlgiOps::W2B2w2]);
   execute(operators[dlgiOps::N2w2]);
   execute(operators[dlgiOps::N3w2]);

   execute(operators[dlgiOps::W2B2b2]);
   execute(operators[dlgiOps::N2b2]);
   execute(operators[dlgiOps::N3b2]);

   execute(operators[dlgiOps::W2B2w3]);
   execute(operators[dlgiOps::N2w3]);
   execute(operators[dlgiOps::N3w3]);

   execute(operators[dlgiOps::W2B2b3]);
   execute(operators[dlgiOps::N2b3]);
   execute(operators[dlgiOps::N3b3]);

   //execute(operators[dlgiOps::w1b1]);
   //execute(operators[dlgiOps::n1]);
   //execute(operators[dlgiOps::w2b2]);
   //execute(operators[dlgiOps::n2]);
   //execute(operators[dlgiOps::w3b3]);
   //execute(operators[dlgiOps::n3]);
}

void rnd_DmlOperator::InitMul(rnd_Tensor& A, rnd_Tensor& B, rnd_Tensor& C, LPCWSTR name)
{
   DML_TENSOR_DESC dmlTensorDescA{ DML_TENSOR_TYPE_BUFFER, &A.tensorDesc };
   DML_TENSOR_DESC dmlTensorDescB{ DML_TENSOR_TYPE_BUFFER, &B.tensorDesc };
   DML_TENSOR_DESC dmlTensorDescC{ DML_TENSOR_TYPE_BUFFER, &C.tensorDesc };

   output.tensorSizes[0] = A.tensorDesc.Sizes[0];
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

   this->name = name;

   bindingProperties = dmlCompiledOperator->GetBindingProperties();

   tensors.assign({ &A, &B, &C });
}

void rnd_DmlOperator::InitTanh(rnd_Tensor& A, LPCWSTR name)
{
   DML_TENSOR_DESC dmlTensorDescA{ DML_TENSOR_TYPE_BUFFER, &A.tensorDesc };

   output.tensorSizes[0] = A.tensorDesc.Sizes[0];
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

   this->name = name;

   bindingProperties = dmlCompiledOperator->GetBindingProperties();

   tensors.push_back(&A);
}

void rnd_DmlOperator::OnInit()
{
   output.OnInit(output.tensorDesc.TotalTensorSizeInBytes, name.c_str());

   if (bindingProperties.PersistentResourceSize)
      persBufferBinding = { output.buffer.Get(), 0, bindingProperties.PersistentResourceSize };
}

void rnd_DmlOperator::Execute()
{

}


void rnd_DynamicTensor::RecalcSizes()
{
   tensorDesc.TotalTensorSizeInBytes = DMLCalcBufferTensorSize(
      tensorDesc.DataType,
      tensorDesc.DimensionCount,
      tensorDesc.Sizes,
      tensorDesc.Strides);
}