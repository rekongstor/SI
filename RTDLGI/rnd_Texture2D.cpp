#include "rnd_Texture2D.h"
#include "rnd_Dx12.h"

float zeroClearValue[] = {0, 0, 0 ,0};
float onesClearValue[] = {1, 1, 1, 1};

void rnd_Texture2D::OnInit(ID3D12Resource* buffer, D3D12_RESOURCE_STATES initialState, LPCWSTR name /*= L""*/)
{
   this->buffer = buffer;
   this->format = buffer->GetDesc().Format;
   this->state = initialState;
   this->flags = buffer->GetDesc().Flags;

   buffer->SetName(name);
}

void rnd_Texture2D::OnInit(DXGI_FORMAT format, Buffer2D dim, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initialState, LPCWSTR name, int mips, float* clearValue)
{
   this->format = format;
   this->width = dim.width;
   this->height = dim.height;
   this->mips = mips;
   this->state = initialState;
   this->flags = flags;

   auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

   auto bufferDesc(CD3DX12_RESOURCE_DESC::Tex2D(format, dim.width, dim.height, 1, mips, 1, 0, flags));

   D3D12_CLEAR_VALUE clearVal;
   clearVal.Format = format;
   memcpy(clearVal.Color, clearValue, sizeof(float) * 4);
      
   ThrowIfFailed(renderer->Device()->CreateCommittedResource(
      &heapProperties,
      D3D12_HEAP_FLAG_NONE,
      &bufferDesc,
      initialState,
      (flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) ? &clearVal : nullptr,
      IID_PPV_ARGS(&buffer)));
   buffer->SetName(name);
}

void rnd_Texture2D::OnInitReadback(rnd_Texture2D defaultTex, D3D12_RESOURCE_STATES initialState, LPCWSTR name)
{
   this->format = defaultTex.format;
   this->width = defaultTex.width;
   this->height = defaultTex.height;
   this->mips = defaultTex.mips;
   this->state = initialState;
   this->flags = defaultTex.flags;

   auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);

   auto bufferDesc(CD3DX12_RESOURCE_DESC::Buffer(defaultTex.buffer->GetDesc().Width, flags));

   ThrowIfFailed(renderer->Device()->CreateCommittedResource(
      &heapProperties,
      D3D12_HEAP_FLAG_NONE,
      &bufferDesc,
      initialState,
      nullptr,
      IID_PPV_ARGS(&buffer)));
   buffer->SetName(name);
}


void rnd_Texture2D::CreateDsv()
{
   ThrowIfFalse(flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
   dsvHandle = renderer->GetDsvHandle();
   renderer->Device()->CreateDepthStencilView(buffer.Get(), nullptr, dsvHandle.first);
   ThrowIfFailed(renderer->Device()->GetDeviceRemovedReason());
}

void rnd_Texture2D::CreateRtv()
{
   ThrowIfFalse(flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
   rtvHandle = renderer->GetRtvHandle();
   renderer->Device()->CreateRenderTargetView(buffer.Get(), nullptr, rtvHandle.first);
   ThrowIfFailed(renderer->Device()->GetDeviceRemovedReason());
}

void rnd_Texture2D::CreateSrv()
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
   desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
   desc.Texture2D.MostDetailedMip = 0;
   desc.Texture2D.MipLevels = mips;
   desc.Format = srvFormat;

   renderer->Device()->CreateShaderResourceView(buffer.Get(), &desc, srvHandle.first);
   ThrowIfFailed(renderer->Device()->GetDeviceRemovedReason());
}

void rnd_Texture2D::CreateUav(int mipSlice)
{
   ThrowIfFalse(flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
   uavHandle[mipSlice] = renderer->GetCbvSrvUavHandle();
   D3D12_UNORDERED_ACCESS_VIEW_DESC desc;
   ZeroMemory(&desc, sizeof(desc));
   desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
   desc.Format = format;
   desc.Texture2D.MipSlice = mipSlice;

   renderer->Device()->CreateUnorderedAccessView(buffer.Get(), nullptr, &desc, uavHandle[mipSlice].first);
   ThrowIfFailed(renderer->Device()->GetDeviceRemovedReason());
}