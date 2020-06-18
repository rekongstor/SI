#pragma once
#include <DirectXMath.h>

#ifdef _WIN64
typedef long long WinProc;
#else
typedef long WinProc;
#endif


using DirectX::XMFLOAT3;
using DirectX::XMFLOAT4;
using DirectX::XMFLOAT4X4;
using DirectX::XMMATRIX;
using DirectX::XMVECTOR;
using DirectX::XMMatrixPerspectiveFovLH;
using DirectX::XMMatrixLookAtLH;
using DirectX::XMStoreFloat4;
using DirectX::XMStoreFloat4x4;
using DirectX::XMLoadFloat4;
using DirectX::XMLoadFloat4x4;
using DirectX::XMMatrixTranslationFromVector;
using DirectX::XMMatrixRotationQuaternion;
using DirectX::XMMatrixTranspose;
using DirectX::XMMatrixIdentity;
using DirectX::XMMatrixScaling;

struct Vertex
{
   XMFLOAT4 pos;
   XMFLOAT4 color;
};