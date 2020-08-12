#include "siDevice.h"

void siDevice::onInit(IDXGIFactory4* factory, D3D_FEATURE_LEVEL featureLevel, bool software)
{
   std::cout << "Initializing device..." << std::endl;
   HRESULT hr = S_OK;

   ComPtr<IDXGIAdapter1> adapter;
   DXGI_ADAPTER_DESC1 desc;
   for (UINT adapterID = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterID, &adapter); ++adapterID)
   {
      hr = adapter->GetDesc1(&desc);
      if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) && software)
         break;
      if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), featureLevel, __uuidof(ID3D12Device), nullptr)) && !software && !(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
         break;
   }

   hr = D3D12CreateDevice(adapter.Get(), featureLevel, IID_PPV_ARGS(&device));
   assert(hr == S_OK);
   device.Get()->SetName(L"Device");
}
