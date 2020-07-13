#include "siPipelineState.h"
#include <d3dcompiler.h>

void siPipelineState::createPso(
   ID3D12Device* device,
   const ComPtr<ID3D12RootSignature>& rootSignature,
   LPCWSTR vsFileName,
   LPCWSTR psFileName,
   const DXGI_FORMAT *rtvFormats,
   uint32_t renderTargetsCount,
   DXGI_SAMPLE_DESC sampleDesc,
   const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputElementDesc)
{
   assert(renderTargetsCount <= 8);
   HRESULT hr;

   ComPtr<ID3DBlob> vertexShader;
   hr = D3DCompileFromFile(vsFileName,
                           nullptr,
                           nullptr,
                           "main",
                           "vs_5_1",
                           D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
                           NULL,
                           &vertexShader,
                           nullptr);
   assert(hr == S_OK);

   ComPtr<ID3DBlob> pixelShader;
   hr = D3DCompileFromFile(psFileName,
                           nullptr,
                           nullptr,
                           "main",
                           "ps_5_1",
                           D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
                           NULL,
                           &pixelShader,
                           nullptr);
   assert(hr == S_OK);

   D3D12_SHADER_BYTECODE vertexShaderByteCode = {vertexShader->GetBufferPointer(), vertexShader->GetBufferSize()};
   D3D12_SHADER_BYTECODE pixelShaderByteCode = {pixelShader->GetBufferPointer(), pixelShader->GetBufferSize()};


   D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {inputElementDesc.data(), inputElementDesc.size()};


   D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
   ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
   psoDesc.InputLayout = inputLayoutDesc;
   psoDesc.pRootSignature = rootSignature.Get();
   psoDesc.VS = vertexShaderByteCode;
   psoDesc.PS = pixelShaderByteCode;
   psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
   for (uint32_t i = 0; i < renderTargetsCount; ++i)
      psoDesc.RTVFormats[i] = rtvFormats[i];
   psoDesc.SampleDesc = sampleDesc;
   psoDesc.SampleMask = UINT_MAX;
   psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
   psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
   psoDesc.NumRenderTargets = renderTargetsCount;
   psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

   hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineStateObject));
   assert(hr == S_OK);
}

const ComPtr<ID3D12PipelineState>& siPipelineState::getPipelineState() const
{
   return pipelineStateObject;
}
