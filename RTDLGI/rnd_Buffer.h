#pragma once

struct Buffer1D
{
   int width;
};

struct Buffer2D
{
   int width;
   int height;
};

struct Buffer3D
{
   int width;
   int height;
   int depth;
};

class rnd_Buffer
{
public:
   ComPtr<ID3D12Resource> buffer;
   D3D12_RESOURCE_STATES state; // atomic?
   DXGI_FORMAT format;

   void SetState(D3D12_RESOURCE_STATES nextState)
   {
      state = nextState;
   }
   virtual void CleanUploadData()
   {
      ThrowMsg(L"This type of buffer cannot be used for UploadResolve!");
   }
};