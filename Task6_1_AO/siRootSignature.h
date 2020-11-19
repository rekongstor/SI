#pragma once
class siRootSignature
{
   ComPtr<ID3D12RootSignature> rootSignature;
public:
   void onInit(ID3D12Device* device, const ComPtr<ID3DBlob>& signature);
   static ComPtr<ID3DBlob> createRtGlobalRootSignature();
   static ComPtr<ID3DBlob> createRtLocalRootSignature();
   static ComPtr<ID3DBlob> createCsRsBlobCb1In1Out(uint32_t inputCount, uint32_t outputCount, D3D12_STATIC_SAMPLER_DESC* samplers, uint32_t samplersCount);
   static ComPtr<ID3DBlob> createSampleRsBlob();

   [[nodiscard]] const ComPtr<ID3D12RootSignature>& get() const;
};

