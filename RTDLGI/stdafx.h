#pragma once
#include <cstdint>
#include <iostream>
#include <map>
#include <initguid.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <d3d12.h>
#include "../Core/d3dx12.h"
#include <DirectXMath.h>
#include <wrl.h>


inline void ThrowIfFailed(HRESULT hr)
{
   if (FAILED(hr)) {
      throw hr;
   }
}

using namespace DirectX;
using Microsoft::WRL::ComPtr;
using Event = Microsoft::WRL::Wrappers::Event;

using float1 = float;
using float2 = XMFLOAT2;
using float3 = XMFLOAT3;
using float4 = XMFLOAT4;
using float4x4 = XMFLOAT4X4;
using uint = unsigned int;
