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
               std::vector<siTexture*> inputs, std::vector<siTexture*> outputs,
               D3D12_GPU_VIRTUAL_ADDRESS constBufferAddress);
   void dispatch(ID3D12GraphicsCommandList* commandList, uint32_t width = 0, uint32_t height = 0);
   void dispatch(ID3D12GraphicsCommandList* commandList, D3D12_GPU_VIRTUAL_ADDRESS constBufferAddress, uint32_t width = 0, uint32_t height = 0);
};
