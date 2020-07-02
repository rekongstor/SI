#pragma once
class siDevice
{
   ComPtr<ID3D12Device> device;

public:
   ID3D12Device* operator->() const { return device.Get(); }
   [[nodiscard]] ID3D12Device* get() const { return device.Get(); }

   void onInit(IDXGIFactory4* factory, D3D_FEATURE_LEVEL featureLevel, bool software);
};

