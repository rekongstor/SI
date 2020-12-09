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

inline void AllocateUploadBuffer(ID3D12Device* pDevice, void* pData, UINT64 datasize, ID3D12Resource** ppResource, const wchar_t* resourceName = nullptr)
{
   auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
   auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(datasize);
   ThrowIfFailed(pDevice->CreateCommittedResource(
      &uploadHeapProperties,
      D3D12_HEAP_FLAG_NONE,
      &bufferDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ,
      nullptr,
      IID_PPV_ARGS(ppResource)));
   if (resourceName) {
      (*ppResource)->SetName(resourceName);
   }
   void* pMappedData;
   (*ppResource)->Map(0, nullptr, &pMappedData);
   memcpy(pMappedData, pData, datasize);
   (*ppResource)->Unmap(0, nullptr);
}

inline void AllocateUAVBuffer(ID3D12Device* pDevice, UINT64 bufferSize, ID3D12Resource** ppResource, D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_COMMON, const wchar_t* resourceName = nullptr)
{
   auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
   auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
   ThrowIfFailed(pDevice->CreateCommittedResource(
      &uploadHeapProperties,
      D3D12_HEAP_FLAG_NONE,
      &bufferDesc,
      initialResourceState,
      nullptr,
      IID_PPV_ARGS(ppResource)));
   if (resourceName) {
      (*ppResource)->SetName(resourceName);
   }
}

inline UINT Align(UINT size, UINT alignment)
{
   return (size + (alignment - 1)) & ~(alignment - 1);
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

struct D3DBuffer
{
   ComPtr<ID3D12Resource> buffer;
   D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle;
   D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle;
};