#pragma once
#include <cstdint>
#include <iostream>

#include <map>

#include <initguid.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include "../Core/d3dx12.h"
#include <DirectXMath.h>
using namespace DirectX;

#include <wrl.h>
using Microsoft::WRL::ComPtr;

const uint32_t maxFrameBufferCount = 3;

using float1 = float;
using float2 = XMFLOAT2;
using float3 = XMFLOAT3;
using float4 = XMFLOAT4;
using float4x4 = XMFLOAT4X4;
using uint = unsigned int;
