#include "siComputeShader.h"

#include <utility>
#include "siRootSignature.h"
#include "siPipelineState.h"
#include "siTexture.h"


void siComputeShader::onInit(ID3D12Device* device, siDescriptorMgr* descMgr, LPCWSTR filename,
                             std::vector<siTexture*> inputs,
                             std::vector<siTexture*> outputs,
                             D3D12_GPU_VIRTUAL_ADDRESS constBufferAddress)
{
   this->constBufferAddress = constBufferAddress;

   for (auto& input : inputs)
   {
      auto& tex = this->inputs.emplace_back(siTexture());
      tex.initFromTexture(*input);
      tex.createSrv(device, descMgr);
   }
   for (auto& output : outputs)
   {
      if ((*output).getMipLevels() == 1)
      {
         auto& tex = this->outputs.emplace_back(siTexture());
         tex.initFromTexture(*output);
         tex.createUav(device, descMgr);
      }
      else
         for (int i = 0; i < (*output).getMipLevels(); ++i)
         {
            auto& tex = this->outputs.emplace_back(siTexture());
            tex.initFromTexture(*output);
            tex.createUav(device, descMgr, i);
         }
   }

   // creating root signature
   {
      D3D12_STATIC_SAMPLER_DESC samplers[5] = {};

      samplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
      samplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
      samplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
      samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
      samplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
      samplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
      samplers[0].MinLOD = 0.0f;
      samplers[0].MaxLOD = D3D12_FLOAT32_MAX;
      samplers[0].MipLODBias = 0;
      samplers[0].MaxAnisotropy = 1;
      samplers[0].ShaderRegister = 0;
      samplers[0].RegisterSpace = 0;
      samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

      samplers[1].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
      samplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
      samplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
      samplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
      samplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
      samplers[1].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
      samplers[1].MinLOD = 0.0f;
      samplers[1].MaxLOD = D3D12_FLOAT32_MAX;
      samplers[1].MipLODBias = 0;
      samplers[1].MaxAnisotropy = 1;
      samplers[1].ShaderRegister = 1;
      samplers[1].RegisterSpace = 0;
      samplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

      samplers[2].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
      samplers[2].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
      samplers[2].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
      samplers[2].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
      samplers[2].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
      samplers[2].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
      samplers[2].MinLOD = 0.0f;
      samplers[2].MaxLOD = D3D12_FLOAT32_MAX;
      samplers[2].MipLODBias = 0;
      samplers[2].MaxAnisotropy = 1;
      samplers[2].ShaderRegister = 2;
      samplers[2].RegisterSpace = 0;
      samplers[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

      samplers[3].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
      samplers[3].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
      samplers[3].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
      samplers[3].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
      samplers[3].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
      samplers[3].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
      samplers[3].MinLOD = 0.0f;
      samplers[3].MaxLOD = D3D12_FLOAT32_MAX;
      samplers[3].MipLODBias = 0;
      samplers[3].MaxAnisotropy = 1;
      samplers[3].ShaderRegister = 3;
      samplers[3].RegisterSpace = 0;
      samplers[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

      samplers[4].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
      samplers[4].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
      samplers[4].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
      samplers[4].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
      samplers[4].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
      samplers[4].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
      samplers[4].MinLOD = 0.0f;
      samplers[4].MaxLOD = D3D12_FLOAT32_MAX;
      samplers[4].MipLODBias = 0;
      samplers[4].MaxAnisotropy = 1;
      samplers[4].ShaderRegister = 4;
      samplers[4].RegisterSpace = 0;
      samplers[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

      rootSignature.onInit(
         device, siRootSignature::createCsRsBlobCb1In1Out(inputs.size(), this->outputs.size(), samplers,
                                                          _countof(samplers)));
   }

   pipelineState.createPso(device, rootSignature.get(), filename);
}

void siComputeShader::dispatch(ID3D12GraphicsCommandList* commandList, uint32_t width, uint32_t height)
{
   dispatch(commandList, constBufferAddress);
}

void siComputeShader::dispatch(ID3D12GraphicsCommandList* commandList, D3D12_GPU_VIRTUAL_ADDRESS constBufferAddress,
                               uint32_t width, uint32_t height)
{
   if (outputs.empty())
      return;
   commandList->SetComputeRootSignature(rootSignature.get().Get());

   for (auto& input : inputs)
      input.resourceBarrier(commandList, D3D12_RESOURCE_STATE_GENERIC_READ);

   for (auto& output : outputs)
      output.resourceBarrier(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

   uint32_t slot = 0;
   commandList->SetComputeRootConstantBufferView(slot++, constBufferAddress);
   commandList->SetComputeRootDescriptorTable(slot++, outputs[0].getUavHandle().second);
   commandList->SetComputeRootDescriptorTable(slot++, inputs[0].getSrvHandle().second);

   commandList->SetPipelineState(pipelineState.getPipelineState().Get());
   if (width == 0)
      width = static_cast<UINT>(ceilf(outputs[0].getWidth() / 8.f));
   if (height == 0)
      height = static_cast<UINT>(ceilf(outputs[0].getHeight() / 8.f));

   commandList->Dispatch(width, height, 1);

   for (auto& input : inputs)
      input.resourceBarrier(commandList, D3D12_RESOURCE_STATE_GENERIC_READ);

   for (auto& output : outputs)
      output.resourceBarrier(commandList, D3D12_RESOURCE_STATE_GENERIC_READ);
}
