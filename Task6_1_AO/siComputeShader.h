#pragma once
class siPipelineState;
class siRootSignature;
class siTexture2D;

class siComputeShader
{
   siRootSignature* rootSignature = nullptr;
   siPipelineState* pipelineState = nullptr;
   siTexture2D* output = nullptr;
   siTexture2D* input = nullptr;
   siTexture2D* input2 = nullptr;
   D3D12_GPU_VIRTUAL_ADDRESS constBufferAddress = 0;

public:
   void onInit(siRootSignature* rootSignature, siPipelineState* pipelineState, siTexture2D* output, siTexture2D* input,
               siTexture2D* input2, D3D12_GPU_VIRTUAL_ADDRESS constBufferAddress);
   void dispatch(ID3D12GraphicsCommandList* commandList, uint32_t width, uint32_t height, D3D12_RESOURCE_STATES outputState, D3D12_RESOURCE_STATES inputState);
};
