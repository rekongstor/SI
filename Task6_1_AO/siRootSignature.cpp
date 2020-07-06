#include "siRootSignature.h"


void siRootSignature::onInit(ID3D12Device* device, const ComPtr<ID3DBlob>& signature)
{
   HRESULT hr = S_OK;

   hr = device->CreateRootSignature(0,
      signature->GetBufferPointer(),
      signature->GetBufferSize(),
      IID_PPV_ARGS(&rootSignature));
   assert(hr == S_OK);
}

ComPtr<ID3DBlob> siRootSignature::createSampleRsBlob()
{
   HRESULT hr = S_OK;

   CD3DX12_ROOT_PARAMETER rootParameters[3];
   rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
   rootParameters[1].InitAsShaderResourceView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
   rootParameters[2].InitAsDescriptorTable(1,
      &CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0, 1),
      D3D12_SHADER_VISIBILITY_ALL);

   ComPtr<ID3DBlob> signature;

   hr = D3D12SerializeRootSignature(
      &CD3DX12_ROOT_SIGNATURE_DESC(
         _countof(rootParameters), rootParameters,
         1, &CD3DX12_STATIC_SAMPLER_DESC(0),
         D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
         D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
         D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
         D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
      ),
      D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr);
   assert(hr == S_OK);

   return signature;
}

const ComPtr<ID3D12RootSignature>& siRootSignature::get() const
{
   return rootSignature;
}
