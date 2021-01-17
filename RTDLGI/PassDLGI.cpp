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
   static const char* W1 = "data\\dlgi\\W1.exr";
   static const char* B2 = "data\\dlgi\\B2.exr";
   static const char* W2 = "data\\dlgi\\W2.exr";
   static const char* B3 = "data\\dlgi\\B3.exr";
   static const char* W3 = "data\\dlgi\\W3.exr";
   static const char* B4 = "data\\dlgi\\B4.exr";
   static const char* W4 = "data\\dlgi\\W4.exr";
   static const char* B5 = "data\\dlgi\\B5.exr";
   static const char* W5 = "data\\dlgi\\W5.exr";
}

namespace dlgiOps
{
   const char* W1B1 = "W1B1";
   const char* N1 = "N1";
   const char* W2B2 = "W2B2";
   const char* N2 = "N2";
   const char* W3B3 = "W3B3";
   const char* N3 = "N3";
   const char* W4B4 = "W4B4";
   const char* N4 = "N4";
   const char* N5 = "N5";
}
#pragma endregion 

using namespace dlgiFiles;
using namespace dlgiOps;

void PassDLGI::OnInit()
{
   const char* inputs[] = {
      W1, W2 ,W3, W4, W5,
      B1, B2 ,B3, B4, B5,
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
      data.resize(((width * height / 2) + (odd ? 1 : 0)) * sizeof(XMFLOAT2));
      XMFLOAT2 tmp;
      //for (int i = 0; i < width; ++i) {
      //   for (int j = 0; j < height; ++j) {
      //      if ((i * height + j) % 2 == 1) // skip every odd value
      //         continue;
      //
      //      float f1 = GetCell(i, j);
      //      float f2 = GetCell(i, j + 1);
      //      XMStoreFloat2(&tmp, { f1, f2 });
      //      XMFLOAT2* dataPtr = (XMFLOAT2*)(data.data());
      //      dataPtr[(i * height + j) / 2] = tmp;
      //   }
      //}
      for (int i = 0; i < width * height / 2; ++i) {
         float f1 = out[i * 2 * 4];
         float f2 = out[(i * 2 + 1) * 4];
         XMStoreFloat2(&tmp, { f1, f2 });
         XMFLOAT2* dataPtr = (XMFLOAT2*)(data.data());
         dataPtr[i] = tmp;
      }
      if (odd)
      {
         XMStoreFloat2(&tmp, {out[width * height / 4 + 1], 0});
         XMFLOAT2* dataPtr = (XMFLOAT2*)(data.data());
         dataPtr[width * height / 4] = tmp;
      }

      auto& tensor = inputTensors[input];

      tensor.tensorSizes[2] = height;
      tensor.tensorSizes[3] = width;

      DML_BUFFER_TENSOR_DESC& dmlBufferTensorDesc = tensor.tensorDesc;
      dmlBufferTensorDesc.DataType = DML_TENSOR_DATA_TYPE_FLOAT32;
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
      dmlBufferTensorDesc.DataType = DML_TENSOR_DATA_TYPE_FLOAT32;
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

   // Operators
   {
      // I think it's easier to preInit all
      operators[W1B1];
      operators[N1];

      operators[W2B2];
      operators[N2];

      operators[W3B3];
      operators[N3];

      operators[W4B4];
      operators[N4];

      operators[N5];

      operators[W1B1].InitMul(inputRtData, inputTensors[W1], inputTensors[B1], L"W1B1");
      operators[N1].InitTanh(operators[W1B1].output, L"N1");

      operators[W2B2].InitMul(operators[N1].output, inputTensors[W2], inputTensors[B2], L"W2B2");
      operators[N2].InitTanh(operators[W2B2].output, L"N2");

      operators[W3B3].InitMul(operators[N2].output, inputTensors[W3], inputTensors[B3], L"W3B3");
      operators[N3].InitTanh(operators[W3B3].output, L"N3");

      operators[W4B4].InitMul(operators[N3].output, inputTensors[W4], inputTensors[B4], L"W4B4");
      operators[N4].InitTanh(operators[W4B4].output, L"N4");

      operators[N5].InitMul(operators[N4].output, inputTensors[W5], inputTensors[B5], L"W5B5");
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
   descriptorHeapDesc.NumDescriptors = descriptorCount * 2;
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

   execute(operators[dlgiOps::W2B2]);
   execute(operators[dlgiOps::N2]);

   execute(operators[dlgiOps::W3B3]);
   execute(operators[dlgiOps::N3]);

   execute(operators[dlgiOps::W4B4]);
   execute(operators[dlgiOps::N4]);

   execute(operators[dlgiOps::N5]);
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
   dmlBufferTensorDesc.DataType = DML_TENSOR_DATA_TYPE_FLOAT32;
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
   ThrowIfFailed(renderer->DmlDevice()->CompileOperator(dmlOperator.Get(), DML_EXECUTION_FLAG_NONE, IID_PPV_ARGS(&dmlCompiledOperator)));

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
   dmlBufferTensorDesc.DataType = DML_TENSOR_DATA_TYPE_FLOAT32;
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

   ThrowIfFailed(renderer->DmlDevice()->CompileOperator(dmlOperator.Get(), DML_EXECUTION_FLAG_NONE, IID_PPV_ARGS(&dmlCompiledOperator)));

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