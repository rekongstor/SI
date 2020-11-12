#pragma once
class siDevice
{
   ComPtr<ID3D12Device6> device;

public:
   ID3D12Device6* operator->() const { return device.Get(); }
   [[nodiscard]] ID3D12Device6* get() const { return device.Get(); }

   void onInit(IDXGIFactory4* factory, D3D_FEATURE_LEVEL featureLevel, bool software);
};

