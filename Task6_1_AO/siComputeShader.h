#pragma once
#include "siPipelineState.h"
#include "siRootSignature.h"
#include "siTexture2D.h"


class siComputeShader
{
   siRootSignature rootSignature;
   siPipelineState pipelineState;
   std::vector<siTexture2D> outputs;
   std::vector<siTexture2D> inputs;
   D3D12_GPU_VIRTUAL_ADDRESS constBufferAddress = 0;

public:
   void onInit(ID3D12Device* device, siDescriptorMgr* descMgr, LPCWSTR filename,
               const std::vector<siTexture2D>& inputs, const std::vector<siTexture2D>& outputs,
               D3D12_GPU_VIRTUAL_ADDRESS constBufferAddress);
   void dispatch(ID3D12GraphicsCommandList* commandList, uint32_t width, uint32_t height);
};
