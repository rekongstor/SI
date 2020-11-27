#include "rnd_Texture.h"

#include "rnd_DescriptorHeapMgr.h"

void rnd_Texture::OnInit(ID3D12Resource* buffer, DXGI_FORMAT format, D3D12_RESOURCE_STATES initialState, LPCWSTR name /*= L""*/)
{
   this->buffer = buffer;
   this->format = format;

   buffer->SetName(name);
}

void rnd_Texture::SetState(D3D12_RESOURCE_STATES nextState)
{
   state = nextState;
}


void rnd_Texture::CreateDsv(ID3D12Device* device, rnd_DescriptorHeapMgr* descMgr)
{
   dsvHandle = descMgr->GetDsvHandle();
   device->CreateDepthStencilView(buffer.Get(), nullptr, dsvHandle.first);
   ThrowIfFailed(device->GetDeviceRemovedReason());
}

void rnd_Texture::CreateRtv(ID3D12Device* device, rnd_DescriptorHeapMgr* descMgr)
{
   rtvHandle = descMgr->GetRtvHandle();
   device->CreateRenderTargetView(buffer.Get(), nullptr, rtvHandle.first);
   ThrowIfFailed(device->GetDeviceRemovedReason());
}

void rnd_Texture::CreateSrv(ID3D12Device* device, rnd_DescriptorHeapMgr* descMgr)
{
   DXGI_FORMAT srvFormat;
   switch (format) {
   case DXGI_FORMAT::DXGI_FORMAT_D16_UNORM:
      srvFormat = DXGI_FORMAT::DXGI_FORMAT_R16_FLOAT;
      break;
   case DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT:
      srvFormat = DXGI_FORMAT::DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
      break;
   case DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT:
      srvFormat = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
      break;
   case DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
      srvFormat = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
      break;
   default:
      srvHandle = descMgr->GetCbvSrvUavHandle();
      device->CreateShaderResourceView(buffer.Get(), nullptr, srvHandle.first);
      return;
   }

   D3D12_SHADER_RESOURCE_VIEW_DESC desc;
   ZeroMemory(&desc, sizeof(desc));
   desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
   desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
   desc.Texture2D.MostDetailedMip = 0;
   desc.Texture2D.MipLevels = 1;
   desc.Format = srvFormat;

   srvHandle = descMgr->GetCbvSrvUavHandle();
   device->CreateShaderResourceView(buffer.Get(), &desc, srvHandle.first);
   ThrowIfFailed(device->GetDeviceRemovedReason());
}

void rnd_Texture::CreateUav(ID3D12Device* device, rnd_DescriptorHeapMgr* descMgr, int32_t mipLevel)
{
   if (mipLevel >= 0) {
      D3D12_UNORDERED_ACCESS_VIEW_DESC desc;
      ZeroMemory(&desc, sizeof(desc));
      desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
      desc.Format = format;
      desc.Texture2DArray.MipSlice = mipLevel;
      desc.Texture2DArray.ArraySize = 4;
      desc.Texture2DArray.FirstArraySlice = 0;

      uavHandle = descMgr->GetCbvSrvUavHandle();
      device->CreateUnorderedAccessView(buffer.Get(), nullptr, &desc, uavHandle.first);
      ThrowIfFailed(device->GetDeviceRemovedReason());
      return;
   }
   uavHandle = descMgr->GetCbvSrvUavHandle();
   device->CreateUnorderedAccessView(buffer.Get(), nullptr, nullptr, uavHandle.first);
   ThrowIfFailed(device->GetDeviceRemovedReason());
}