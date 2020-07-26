#pragma once
#include "siPipelineState.h"
#include "siRootSignature.h"
#include "siTexture.h"


class siComputeShader
{
   siRootSignature rootSignature;
   siPipelineState pipelineState;
   std::vector<siTexture> outputs;
   std::vector<siTexture> inputs;
   D3D12_GPU_VIRTUAL_ADDRESS constBufferAddress = 0;

public:
   void onInit(ID3D12Device* device, siDescriptorMgr* descMgr, LPCWSTR filename,
               const std::vector<siTexture>& inputs, const std::vector<siTexture>& outputs,
               D3D12_GPU_VIRTUAL_ADDRESS constBufferAddress);
   void dispatch(ID3D12GraphicsCommandList* commandList);
   void dispatch(ID3D12GraphicsCommandList* commandList, D3D12_GPU_VIRTUAL_ADDRESS constBufferAddress);
};
