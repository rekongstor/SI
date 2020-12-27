#pragma once
#include <cstdint>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <iosfwd>
#include <corecrt_math_defines.h>

#include <initguid.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include "../Core/d3dx12.h"
#include <wrl.h>

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <queue>


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

class rnd_Buffer;

using DescHandlePair = std::pair<CD3DX12_CPU_DESCRIPTOR_HANDLE, CD3DX12_GPU_DESCRIPTOR_HANDLE>;
using UploadPair = std::pair<ComPtr<ID3D12Resource>, rnd_Buffer*>;

struct State
{
   uint64_t state = 0;

   State() = default;
   State(uint64_t st) : state(st) {}
   bool Is(uint64_t st) { return state & st; }
   void Set(uint64_t st) { state |= st; }
};


#define FRAME_COUNT 2
#define MAX_MIP 16
#define SizeOfInUint32(obj) ((sizeof(obj) - 1) / sizeof(UINT32) + 1)


class rnd_Dx12;
class core_Imgui;
class core_Window;

extern core_Imgui* imgui;
extern core_Window* window;
extern rnd_Dx12* renderer;

extern wchar_t nameBuffer[4096];

inline void ThrowIfFailed(HRESULT hr, const wchar_t* errMsg = L"")
{
   if (FAILED(hr)) {
      std::wcout << errMsg << std::endl;
      throw hr;
   }
}

inline void ThrowIfFalse(bool value, const wchar_t* errMsg = L"")
{
   ThrowIfFailed(value ? S_OK : E_FAIL, errMsg);
}

inline void ThrowMsg(const wchar_t* errMsg = L"")
{
   ThrowIfFailed(E_FAIL, errMsg);
}

inline wchar_t* FormatWStr(LPCWSTR format, ...)
{
   va_list _ArgList;
   __crt_va_start(_ArgList, format);
   vswprintf(nameBuffer, _countof(nameBuffer), format, _ArgList);
   __crt_va_end(_ArgList);

   return nameBuffer;
}

constexpr UINT AlignConst(UINT size, UINT alignment)
{
   return (size + (alignment - 1)) & ~(alignment - 1);
}

inline UINT Align(UINT size, UINT alignment)
{
   return (size + (alignment - 1)) & ~(alignment - 1);
}