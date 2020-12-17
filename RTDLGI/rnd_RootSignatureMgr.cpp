#include "rnd_RootSignatureMgr.h"

#include "CubeConstBuf.h"
#include "rnd_Dx12.h"

void rnd_RootSignatureMgr::OnInit()
{

}

ID3D12RootSignature* rnd_RootSignatureMgr::CreateRootSignature(std::initializer_list<D3D12_ROOT_PARAMETER> parameters, std::initializer_list<D3D12_STATIC_SAMPLER_DESC> staticSamplers, D3D12_ROOT_SIGNATURE_FLAGS flags)
{
   ID3D12RootSignature* rootSig;
   D3D12_ROOT_SIGNATURE_DESC desc{ parameters.size(), parameters.begin(), staticSamplers.size(), staticSamplers.begin(), flags };
   ComPtr<ID3DBlob> blob;
   ComPtr<ID3DBlob> error;

   ThrowIfFailed(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error), error ? static_cast<wchar_t*>(error->GetBufferPointer()) : nullptr);
   ThrowIfFailed(renderer->device->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&rootSig)));
   return rootSig;
}
