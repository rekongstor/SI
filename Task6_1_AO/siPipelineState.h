#pragma once

class siPipelineState
{
   ComPtr<ID3D12PipelineState> pipelineStateObject;

public:
   void createPso(ID3D12Device* device, const ComPtr<ID3D12RootSignature>& rootSignature, LPCWSTR vsFileName,
                  LPCWSTR psFileName, DXGI_FORMAT format, DXGI_SAMPLE_DESC sampleDesc,
                  const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputElementDesc);
   [[nodiscard]] const ComPtr<ID3D12PipelineState>& getPipelineState() const;
};
