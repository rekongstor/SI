#pragma once
class siRootSignature
{
   ComPtr<ID3D12RootSignature> rootSignature;
public:
   void onInit(ID3D12Device* device, const ComPtr<ID3DBlob>& signature);
   static ComPtr<ID3DBlob> createCsRsBlobCb1In1Out();
   static ComPtr<ID3DBlob> createSampleRsBlob();

   [[nodiscard]] const ComPtr<ID3D12RootSignature>& get() const;
};

