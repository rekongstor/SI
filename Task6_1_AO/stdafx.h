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
