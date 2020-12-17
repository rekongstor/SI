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

struct D3DBuffer
{
   ComPtr<ID3D12Resource> buffer;
   D3D12_RESOURCE_STATES state; // atomic?
   DXGI_FORMAT format;
};