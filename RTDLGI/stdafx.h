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
#include <string>
#include <unordered_map>
#include <sstream>
#include <iomanip>


using namespace DirectX;
using Microsoft::WRL::ComPtr;
using Event = Microsoft::WRL::Wrappers::Event;

using float1 = float;
using float2 = XMFLOAT2;
using float3 = XMFLOAT3;
using float4 = XMFLOAT4;
using float4x4 = XMFLOAT4X4;
using float3x4 = XMFLOAT3X4;
using float4x3 = XMFLOAT4X3;
using uint = unsigned int;

using DescHandlePair = std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE>;


#define FRAME_COUNT 2
#define SizeOfInUint32(obj) ((sizeof(obj) - 1) / sizeof(UINT32) + 1)


class rnd_Dx12;
extern rnd_Dx12* renderer;

inline void ThrowIfFailed(HRESULT hr, const wchar_t* errMsg = L"")
{
   if (FAILED(hr)) {
      std::wcout << errMsg << std::endl;
      throw hr;
   }
}


inline void ThrowIfFalse(bool value)
{
   ThrowIfFailed(value ? S_OK : E_FAIL);
}


constexpr UINT AlignConst(UINT size, UINT alignment)
{
   return (size + (alignment - 1)) & ~(alignment - 1);
}


inline UINT Align(UINT size, UINT alignment)
{
   return (size + (alignment - 1)) & ~(alignment - 1);
}


struct D3DBuffer
{
   ComPtr<ID3D12Resource> buffer;
   D3D12_RESOURCE_STATES state; // atomic?
};

struct SrvBuffer : D3DBuffer
{
   DescHandlePair srvDescHandle;
};