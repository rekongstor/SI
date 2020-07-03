#pragma once
class siPipelineState
{
   ComPtr<ID3D12RootSignature> rootSignature;
   ComPtr<ID3D12PipelineState> pipelineState;
public:
   void onInit(ID3D12Device* device, const ComPtr<ID3DBlob>& signature);
   ComPtr<ID3DBlob> createSampleRsBlob();

   [[nodiscard]] const ComPtr<ID3D12RootSignature>& getRootSignature() const;
   [[nodiscard]] const ComPtr<ID3D12PipelineState>& getPipelineState() const;
};

