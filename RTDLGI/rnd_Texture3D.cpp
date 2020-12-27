#include "rnd_Texture3D.h"
#include "rnd_Dx12.h"

void rnd_Texture3D::OnInit(DXGI_FORMAT format, Buffer3D dim, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initialState, LPCWSTR name, int mips)
{
   this->format = format;
   this->width = dim.width;
   this->height = dim.height;
   this->mips = mips;
   this->state = initialState;
   this->flags = flags;

   auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

   auto bufferDesc(CD3DX12_RESOURCE_DESC::Tex3D(format, dim.width, dim.height, dim.depth, mips, flags));

   ThrowIfFailed(renderer->Device()->CreateCommittedResource(
      &heapProperties,
      D3D12_HEAP_FLAG_NONE,
      &bufferDesc,
      initialState,
      nullptr,
      IID_PPV_ARGS(&buffer)));
   buffer->SetName(name);
}

void rnd_Texture3D::SetState(D3D12_RESOURCE_STATES nextState)
{
   state = nextState;
}

void rnd_Texture3D::CreateSrv()
{
   ThrowIfFalse(!(flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE));
   srvHandle = renderer->GetCbvSrvUavHandle();

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
      renderer->Device()->CreateShaderResourceView(buffer.Get(), nullptr, srvHandle.first);
      return;
   }

   D3D12_SHADER_RESOURCE_VIEW_DESC desc;
   ZeroMemory(&desc, sizeof(desc));
   desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
   desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
   desc.Texture3D.MipLevels = mips;
   desc.Texture3D.MostDetailedMip = 0;
   desc.Format = srvFormat;

   renderer->Device()->CreateShaderResourceView(buffer.Get(), &desc, srvHandle.first);
   ThrowIfFailed(renderer->Device()->GetDeviceRemovedReason());
}

void rnd_Texture3D::CreateUav(int mipSlice)
{
   ThrowIfFalse(flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
   uavHandle[mipSlice] = renderer->GetCbvSrvUavHandle();
   D3D12_UNORDERED_ACCESS_VIEW_DESC desc;
   ZeroMemory(&desc, sizeof(desc));
   desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
   desc.Format = format;
   desc.Texture3D.MipSlice = mipSlice;
   desc.Texture3D.FirstWSlice = 0;
   desc.Texture3D.WSize = depth;

   renderer->Device()->CreateUnorderedAccessView(buffer.Get(), nullptr, &desc, uavHandle[mipSlice].first);
   ThrowIfFailed(renderer->Device()->GetDeviceRemovedReason());
}