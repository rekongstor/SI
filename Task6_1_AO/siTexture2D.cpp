#include "siTexture2D.h"
#include <wincodec.h>
#include <codecvt>
#include "siCommandList.h"
#include "siDescriptorMgr.h"

void siTexture2D::initFromBuffer(ComPtr<ID3D12Resource>& existingBuffer, DXGI_FORMAT format,
                                 DXGI_SAMPLE_DESC sampleDesc)
{
   buffer = existingBuffer;
   this->format = format;
   this->sampleDesc = sampleDesc;
}

void siTexture2D::initDepthStencil(ID3D12Device* device, uint32_t width, uint32_t height)
{
   HRESULT hr = S_OK;

   D3D12_CLEAR_VALUE optClearValue;
   ZeroMemory(&optClearValue, sizeof(D3D12_CLEAR_VALUE));
   optClearValue.Format = DXGI_FORMAT_D32_FLOAT;
   optClearValue.DepthStencil = {1.f, 0};

   hr = device->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
      D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Tex2D(
         DXGI_FORMAT_D32_FLOAT,
         width,
         height,
         1, 1, 1, 0,
         D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
      D3D12_RESOURCE_STATE_DEPTH_WRITE,
      &optClearValue,
      IID_PPV_ARGS(&buffer)
   );
   assert(hr == S_OK);

   this->sampleDesc = {1, 0};
   this->format = DXGI_FORMAT_D32_FLOAT;
}

void siTexture2D::initTexture(ID3D12Device* device, uint32_t width, uint32_t height,
                              DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState,
                              DXGI_SAMPLE_DESC sampleDesc)
{
   HRESULT hr = S_OK;

   hr = device->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
      D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Tex2D(
         format,
         width,
         height,
         1, 1, sampleDesc.Count, sampleDesc.Quality,
         flags),
      initState,
      nullptr,
      IID_PPV_ARGS(&buffer)
   );
   assert(hr == S_OK);

   this->sampleDesc = sampleDesc;
   this->format = format;
}

void siTexture2D::initFromFile(ID3D12Device* device, std::string_view filename, const siCommandList& commandList)
{
   HRESULT hr = S_OK;
   IWICImagingFactory* wicFactory;
   CoInitialize(nullptr);
   hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory));
   assert(hr == S_OK);

   IWICBitmapDecoder* wicDecoder;
   std::wstring filenameW(filename.begin(), filename.end());
   hr = wicFactory->CreateDecoderFromFilename(filenameW.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad,
                                              &wicDecoder);
   assert(hr == S_OK);

   IWICBitmapFrameDecode* wicFrame;
   hr = wicDecoder->GetFrame(0, &wicFrame);
   assert(hr == S_OK);

   WICPixelFormatGUID pixelFormat;
   hr = wicFrame->GetPixelFormat(&pixelFormat);
   assert(hr == S_OK);

   auto GetDXGIFormatFromWICFormat = [](WICPixelFormatGUID& wicFormatGUID)
   {
      if (wicFormatGUID == GUID_WICPixelFormat128bppRGBAFloat)
         return DXGI_FORMAT_R32G32B32A32_FLOAT;
      if (wicFormatGUID == GUID_WICPixelFormat64bppRGBAHalf)
         return DXGI_FORMAT_R16G16B16A16_FLOAT;
      if (wicFormatGUID == GUID_WICPixelFormat64bppRGBA)
         return DXGI_FORMAT_R16G16B16A16_UNORM;
      if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA)
         return DXGI_FORMAT_R8G8B8A8_UNORM;
      if (wicFormatGUID == GUID_WICPixelFormat32bppBGRA)
         return DXGI_FORMAT_B8G8R8A8_UNORM;
      if (wicFormatGUID == GUID_WICPixelFormat32bppBGR)
         return DXGI_FORMAT_B8G8R8X8_UNORM;
      if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102XR)
         return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;
      if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102)
         return DXGI_FORMAT_R10G10B10A2_UNORM;
      if (wicFormatGUID == GUID_WICPixelFormat16bppBGRA5551)
         return DXGI_FORMAT_B5G5R5A1_UNORM;
      if (wicFormatGUID == GUID_WICPixelFormat16bppBGR565)
         return DXGI_FORMAT_B5G6R5_UNORM;
      if (wicFormatGUID == GUID_WICPixelFormat32bppGrayFloat)
         return DXGI_FORMAT_R32_FLOAT;
      if (wicFormatGUID == GUID_WICPixelFormat16bppGrayHalf)
         return DXGI_FORMAT_R16_FLOAT;
      if (wicFormatGUID == GUID_WICPixelFormat16bppGray)
         return DXGI_FORMAT_R16_UNORM;
      if (wicFormatGUID == GUID_WICPixelFormat8bppGray)
         return DXGI_FORMAT_R8_UNORM;
      if (wicFormatGUID == GUID_WICPixelFormat8bppAlpha)
         return DXGI_FORMAT_A8_UNORM;
      return DXGI_FORMAT_UNKNOWN;
   };
   auto GetConvertToWICFormat = [](WICPixelFormatGUID& wicFormatGUID) -> WICPixelFormatGUID
   {
      if (wicFormatGUID == GUID_WICPixelFormatBlackWhite)
         return GUID_WICPixelFormat8bppGray;
      if (wicFormatGUID == GUID_WICPixelFormat1bppIndexed)
         return GUID_WICPixelFormat32bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat2bppIndexed)
         return GUID_WICPixelFormat32bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat4bppIndexed)
         return GUID_WICPixelFormat32bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat8bppIndexed)
         return GUID_WICPixelFormat32bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat2bppGray)
         return GUID_WICPixelFormat8bppGray;
      if (wicFormatGUID == GUID_WICPixelFormat4bppGray)
         return GUID_WICPixelFormat8bppGray;
      if (wicFormatGUID == GUID_WICPixelFormat16bppGrayFixedPoint)
         return GUID_WICPixelFormat16bppGrayHalf;
      if (wicFormatGUID == GUID_WICPixelFormat32bppGrayFixedPoint)
         return GUID_WICPixelFormat32bppGrayFloat;
      if (wicFormatGUID == GUID_WICPixelFormat16bppBGR555)
         return GUID_WICPixelFormat16bppBGRA5551;
      if (wicFormatGUID == GUID_WICPixelFormat32bppBGR101010)
         return GUID_WICPixelFormat32bppRGBA1010102;
      if (wicFormatGUID == GUID_WICPixelFormat24bppBGR)
         return GUID_WICPixelFormat32bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat24bppRGB)
         return GUID_WICPixelFormat32bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat32bppPBGRA)
         return GUID_WICPixelFormat32bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat32bppPRGBA)
         return GUID_WICPixelFormat32bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat48bppRGB)
         return GUID_WICPixelFormat64bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat48bppBGR)
         return GUID_WICPixelFormat64bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat64bppBGRA)
         return GUID_WICPixelFormat64bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBA)
         return GUID_WICPixelFormat64bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat64bppPBGRA)
         return GUID_WICPixelFormat64bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat48bppRGBFixedPoint)
         return GUID_WICPixelFormat64bppRGBAHalf;
      if (wicFormatGUID == GUID_WICPixelFormat48bppBGRFixedPoint)
         return GUID_WICPixelFormat64bppRGBAHalf;
      if (wicFormatGUID == GUID_WICPixelFormat64bppRGBAFixedPoint)
         return GUID_WICPixelFormat64bppRGBAHalf;
      if (wicFormatGUID == GUID_WICPixelFormat64bppBGRAFixedPoint)
         return GUID_WICPixelFormat64bppRGBAHalf;
      if (wicFormatGUID == GUID_WICPixelFormat64bppRGBFixedPoint)
         return GUID_WICPixelFormat64bppRGBAHalf;
      if (wicFormatGUID == GUID_WICPixelFormat64bppRGBHalf)
         return GUID_WICPixelFormat64bppRGBAHalf;
      if (wicFormatGUID == GUID_WICPixelFormat48bppRGBHalf)
         return GUID_WICPixelFormat64bppRGBAHalf;
      if (wicFormatGUID == GUID_WICPixelFormat128bppPRGBAFloat)
         return GUID_WICPixelFormat128bppRGBAFloat;
      if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFloat)
         return GUID_WICPixelFormat128bppRGBAFloat;
      if (wicFormatGUID == GUID_WICPixelFormat128bppRGBAFixedPoint)
         return GUID_WICPixelFormat128bppRGBAFloat;
      if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFixedPoint)
         return GUID_WICPixelFormat128bppRGBAFloat;
      if (wicFormatGUID == GUID_WICPixelFormat32bppRGBE)
         return GUID_WICPixelFormat128bppRGBAFloat;
      if (wicFormatGUID == GUID_WICPixelFormat32bppCMYK)
         return GUID_WICPixelFormat32bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat64bppCMYK)
         return GUID_WICPixelFormat64bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat40bppCMYKAlpha)
         return GUID_WICPixelFormat64bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat80bppCMYKAlpha)
         return GUID_WICPixelFormat64bppRGBA;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
      if (wicFormatGUID == GUID_WICPixelFormat32bppRGB)
         return GUID_WICPixelFormat32bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat64bppRGB)
         return GUID_WICPixelFormat64bppRGBA;
      if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBAHalf)
         return GUID_WICPixelFormat64bppRGBAHalf;
#endif
      return GUID_WICPixelFormatDontCare;
   };
   auto GetDXGIFormatBitsPerPixel = [](DXGI_FORMAT& dxgiFormat)
   {
      if (dxgiFormat == DXGI_FORMAT_R32G32B32A32_FLOAT)
         return 128;
      if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_FLOAT)
         return 64;
      if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_UNORM)
         return 64;
      if (dxgiFormat == DXGI_FORMAT_R8G8B8A8_UNORM)
         return 32;
      if (dxgiFormat == DXGI_FORMAT_B8G8R8A8_UNORM)
         return 32;
      if (dxgiFormat == DXGI_FORMAT_B8G8R8X8_UNORM)
         return 32;
      if (dxgiFormat == DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM)
         return 32;
      if (dxgiFormat == DXGI_FORMAT_R10G10B10A2_UNORM)
         return 32;
      if (dxgiFormat == DXGI_FORMAT_B5G5R5A1_UNORM)
         return 16;
      if (dxgiFormat == DXGI_FORMAT_B5G6R5_UNORM)
         return 16;
      else
      {
         if (dxgiFormat == DXGI_FORMAT_R32_FLOAT)
            return 32;
         if (dxgiFormat == DXGI_FORMAT_R16_FLOAT)
            return 16;
         if (dxgiFormat == DXGI_FORMAT_R16_UNORM)
            return 16;
         if (dxgiFormat == DXGI_FORMAT_R8_UNORM)
            return 8;
         if (dxgiFormat == DXGI_FORMAT_A8_UNORM)
            return 8;
      }
   };

   auto dxgiFormat = GetDXGIFormatFromWICFormat(pixelFormat);
   bool imageConverted = false;
   IWICFormatConverter* wicFormatConverter = nullptr;
   if (dxgiFormat == DXGI_FORMAT_UNKNOWN)
   {
      auto newPixelFormat = GetConvertToWICFormat(pixelFormat);
      assert(newPixelFormat != GUID_WICPixelFormatDontCare);

      dxgiFormat = GetDXGIFormatFromWICFormat(newPixelFormat);
      hr = wicFactory->CreateFormatConverter(&wicFormatConverter);
      assert(hr == S_OK);

      BOOL canConvert;
      hr = wicFormatConverter->CanConvert(pixelFormat, newPixelFormat, &canConvert);
      assert(hr == S_OK);
      assert(canConvert == TRUE);

      hr = wicFormatConverter->Initialize(wicFrame,
                                          newPixelFormat,
                                          WICBitmapDitherTypeErrorDiffusion,
                                          nullptr,
                                          0,
                                          WICBitmapPaletteTypeCustom);
      assert(hr == S_OK);
      imageConverted = true;
   }
   int bitsPerPixel = GetDXGIFormatBitsPerPixel(dxgiFormat);
   UINT width, height;
   wicFrame->GetSize(&width, &height);
   int bytesPerRow = (width * bitsPerPixel) / 8;
   int imageSize = bytesPerRow * height;

   auto& tex = *this;


   data.resize(imageSize);
   if (imageConverted)
      wicFormatConverter->CopyPixels(nullptr, bytesPerRow, imageSize, data.data());
   else
      wicFrame->CopyPixels(nullptr, bytesPerRow, imageSize, data.data());

   D3D12_RESOURCE_DESC desc;
   desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
   desc.Alignment = 0;
   desc.Width = width;
   desc.Height = height;
   desc.DepthOrArraySize = 1;
   desc.MipLevels = 1;
   desc.Format = dxgiFormat;
   desc.SampleDesc.Count = 1;
   desc.SampleDesc.Quality = 0;
   desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
   desc.Flags = D3D12_RESOURCE_FLAG_NONE;


   hr = device->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
      D3D12_HEAP_FLAG_NONE,
      &desc,
      D3D12_RESOURCE_STATE_COPY_DEST,
      nullptr,
      IID_PPV_ARGS(&buffer));
   assert(hr == S_OK);


   const UINT64 uploadBufferSize = GetRequiredIntermediateSize(buffer.Get(), 0, 1);

   hr = device->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
      D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(&textureUploadHeap));
   assert(hr == S_OK);

   commandList.updateSubresource(buffer.Get(), textureUploadHeap.Get(),
                                 {data.data(), bytesPerRow, bytesPerRow * desc.Height});

   sampleDesc = desc.SampleDesc;
   format = desc.Format;
}

void siTexture2D::releaseUploadBuffer()
{
   textureUploadHeap.Reset();
   data.clear();
}


void siTexture2D::createDsv(ID3D12Device* device, siDescriptorMgr* descMgr)
{
   D3D12_DEPTH_STENCIL_VIEW_DESC desc;
   ZeroMemory(&desc, sizeof(desc));
   desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
   desc.Format = format;
   desc.Flags = D3D12_DSV_FLAG_NONE;

   dsvHandle = descMgr->getDsvHandle();
   device->CreateDepthStencilView(buffer.Get(), &desc, dsvHandle.first);
   auto hr = device->GetDeviceRemovedReason();
   assert(hr == S_OK);
}

auto siTexture2D::createRtv(ID3D12Device* device, siDescriptorMgr* descMgr) -> void
{
   rtvHandle = descMgr->getRtvHandle();
   device->CreateRenderTargetView(buffer.Get(), nullptr, rtvHandle.first);
   auto hr = device->GetDeviceRemovedReason();
   assert(hr == S_OK);
}

void siTexture2D::createSrv(ID3D12Device* device, siDescriptorMgr* descMgr)
{
   DXGI_FORMAT srvformat;
   switch (format)
   {
   case DXGI_FORMAT::DXGI_FORMAT_D16_UNORM:
      srvformat = DXGI_FORMAT::DXGI_FORMAT_R16_FLOAT;
      break;
   case DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT:
      srvformat = DXGI_FORMAT::DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
      break;
   case DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT:
      srvformat = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
      break;
   case DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
      srvformat = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
      break;
   default:
      srvformat = format;
   }

   D3D12_SHADER_RESOURCE_VIEW_DESC desc;
   ZeroMemory(&desc, sizeof(desc));
   desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
   desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
   desc.Texture2D.MostDetailedMip = 0;
   desc.Texture2D.MipLevels = 1;
   desc.Format = srvformat;

   srvHandle = descMgr->getCbvSrvUavHandle();
   device->CreateShaderResourceView(buffer.Get(), &desc, srvHandle.first);
   auto hr = device->GetDeviceRemovedReason();
   assert(hr == S_OK);
}

void siTexture2D::createUav(ID3D12Device* device, siDescriptorMgr* descMgr)
{
   D3D12_UNORDERED_ACCESS_VIEW_DESC desc;
   ZeroMemory(&desc, sizeof(desc));
   desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
   desc.Format = format;

   uavHandle = descMgr->getCbvSrvUavHandle();
   device->CreateUnorderedAccessView(buffer.Get(), nullptr, &desc, uavHandle.first);
   auto hr = device->GetDeviceRemovedReason();
   assert(hr == S_OK);
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
