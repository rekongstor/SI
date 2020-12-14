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

#ifndef RAYTRACINGHLSLCOMPAT_H
#define RAYTRACINGHLSLCOMPAT_H

#define RAYGEN_SHADER RayGenShader
#define CLOSEST_HIT_SHADER ClosestHitShader
#define MISS_SHADER MissShader

#ifdef HLSL
#include "HlslCompat.h"

#define HIT_GROUP_FUNC void ##HIT_GROUP ()
#define MAKE_FN_NAME(x) void  _ ## x 
#define FUNCTION_NAME(signal) MAKE_FN_NAME(signal)

#else
using namespace DirectX;

// Shader will use byte encoding to access indices.
typedef UINT16 Index;

#define Q(x) #x
#define QUOTE(x) Q(x)
#define HIT_GROUP HitGroup

static const wchar_t* rayGenShaderName = L"_" QUOTE(RAYGEN_SHADER);
static const wchar_t* closestHitShaderName = L"_" QUOTE(CLOSEST_HIT_SHADER);
static const wchar_t* missShaderName = L"_" QUOTE(MISS_SHADER);

static const wchar_t* hitGroupName = L"_" QUOTE(HIT_GROUP);

#undef HIT_GROUP
#undef RAYGEN_SHADER
#undef CLOSEST_HIT_SHADER
#undef MISS_SHADER
#endif


struct Vertex
{
    XMFLOAT3 position;
    XMFLOAT3 normal;
};

#endif // RAYTRACINGHLSLCOMPAT_H