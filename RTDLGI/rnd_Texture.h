#pragma once
class rnd_Texture
{
public:
   ComPtr<ID3D12Resource> buffer;
   DXGI_FORMAT format;

   void OnInit(ID3D12Resource* buffer, DXGI_FORMAT format, LPCWSTR name = L"");
};

