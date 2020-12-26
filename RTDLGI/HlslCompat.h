//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#ifndef HLSLCOMPAT_H
#define HLSLCOMPAT_H

#ifndef HLSL

#define SEM(semantic)
typedef UINT16 Index;

#else

typedef float2 XMFLOAT2;
typedef float3 XMFLOAT3;
typedef float4 XMFLOAT4;
typedef float4 XMVECTOR;
typedef float4x4 XMMATRIX;
typedef float4x4 XMFLOAT4X4;
typedef uint UINT;
#define SEM(semantic) : semantic

#endif

struct Vertex
{
   XMFLOAT3 position SEM(SV_POSITION0);
   XMFLOAT3 normal SEM(NORMAL0);
   XMFLOAT2 uv SEM(TEXCOORD0);
};

#endif // HLSLCOMPAT_H