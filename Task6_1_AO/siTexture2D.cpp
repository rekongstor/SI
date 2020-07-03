#include "siTexture2D.h"
#include "siDescriptorMgr.h"

void siTexture2D::initFromBuffer(ComPtr<ID3D12Resource>& existingBuffer)
{
   buffer = existingBuffer;
}

void siTexture2D::initDepthStencil(ID3D12Device* device, uint32_t width, uint32_t height)
{
   HRESULT hr = S_OK;

   D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
   ZeroMemory(&depthStencilViewDesc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));
   depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
   depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
   depthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;

   D3D12_CLEAR_VALUE depthStencilClearValue;
   ZeroMemory(&depthStencilClearValue, sizeof(D3D12_CLEAR_VALUE));
   depthStencilClearValue.Format = DXGI_FORMAT_D32_FLOAT;
   depthStencilClearValue.DepthStencil = {1.f, 0};

   hr = device->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
      D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Tex2D(
         DXGI_FORMAT_D32_FLOAT,
         width,
         height,
         1, 0, 1, 0,
         D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
      D3D12_RESOURCE_STATE_DEPTH_WRITE,
      &depthStencilClearValue,
      IID_PPV_ARGS(&buffer)
   );
   assert(hr == S_OK);
}

void siTexture2D::createDsv(ID3D12Device* device, siDescriptorMgr* descMgr)
{
   dsvHandle = descMgr->getDsvHandle();
   device->CreateDepthStencilView(buffer.Get(), nullptr, dsvHandle.first);
}

void siTexture2D::createRtv(ID3D12Device* device, siDescriptorMgr* descMgr)
{
   rtvHandle = descMgr->getRtvHandle();
   device->CreateRenderTargetView(buffer.Get(), nullptr, rtvHandle.first);
}

void siTexture2D::createSrv(ID3D12Device* device, siDescriptorMgr* descMgr)
{
   srvHandle = descMgr->getCbvSrvUavHandle();
   device->CreateShaderResourceView(buffer.Get(), nullptr, srvHandle.first);
}

void siTexture2D::createUav(ID3D12Device* device, siDescriptorMgr* descMgr)
{
   uavHandle = descMgr->getCbvSrvUavHandle();
   device->CreateUnorderedAccessView(buffer.Get(), nullptr, nullptr, uavHandle.first);
}

const ComPtr<ID3D12Resource>& siTexture2D::getBuffer() const
{
   return buffer;
}

const std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE>& siTexture2D::getDsvHandle() const
{
   return dsvHandle;
}

const std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE>& siTexture2D::getRtvHandle() const
{
   return rtvHandle;
}

const std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE>& siTexture2D::getSrvHandle() const
{
   return srvHandle;
}

const std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE>& siTexture2D::getUavHandle() const
{
   return uavHandle;
}
