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
#ifndef RAYTRACING_HLSL
#define RAYTRACING_HLSL

#define HLSL
#include "HlslCompat.h"
#include "RaytracingHlslCompat.h"
#include "SceneConstBuf.h"
#include "CubeConstBuf.h"

// Global root signature
RWTexture2D<float4> RenderTarget : register(u0);

RaytracingAccelerationStructure Scene : register(t0, space0);


cbuffer g_sceneCB : register(b0)
{
   SceneConstantBuffer g_sceneCB;
}

ByteAddressBuffer Indices : register(t1, space0);
StructuredBuffer<Vertex> Vertices : register(t2, space0);

// Local root signature

cbuffer g_cubeCB : register(b1, space1)
{
   CubeConstantBuffer g_cubeCB;
}

// Load three 16 bit indices from a byte addressed buffer.
uint3 Load3x16BitIndices(uint offsetBytes)
{
    uint3 indices;

    // ByteAdressBuffer loads must be aligned at a 4 byte boundary.
    // Since we need to read three 16 bit indices: { 0, 1, 2 } 
    // aligned at a 4 byte boundary as: { 0 1 } { 2 0 } { 1 2 } { 0 1 } ...
    // we will load 8 bytes (~ 4 indices { a b | c d }) to handle two possible index triplet layouts,
    // based on first index's offsetBytes being aligned at the 4 byte boundary or not:
    //  Aligned:     { 0 1 | 2 - }
    //  Not aligned: { - 0 | 1 2 }
    const uint dwordAlignedOffset = offsetBytes & ~3;    
    const uint2 four16BitIndices = Indices.Load2(dwordAlignedOffset);
 
    // Aligned: { 0 1 | 2 - } => retrieve first three 16bit indices
    if (dwordAlignedOffset == offsetBytes)
    {
        indices.x = four16BitIndices.x & 0xffff;
        indices.y = (four16BitIndices.x >> 16) & 0xffff;
        indices.z = four16BitIndices.y & 0xffff;
    }
    else // Not aligned: { - 0 | 1 2 } => retrieve last three 16bit indices
    {
        indices.x = (four16BitIndices.x >> 16) & 0xffff;
        indices.y = four16BitIndices.y & 0xffff;
        indices.z = (four16BitIndices.y >> 16) & 0xffff;
    }

    return indices;
}

struct MyAttributes { float2 barycentrics; };
struct RayPayload
{
    float4 color;
};

// Retrieve hit world position.
float3 HitWorldPosition()
{
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

// Retrieve attribute at a hit position interpolated from vertex attributes using the hit's barycentrics.
float3 HitAttribute(float3 vertexAttribute[3], MyAttributes attr)
{
    return vertexAttribute[0] +
        attr.barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) +
        attr.barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
}

// Generate a ray in world space for a camera pixel corresponding to an index from the dispatched 2D grid.
inline void GenerateCameraRay(uint2 index, out float3 origin, out float3 direction)
{
    float2 xy = index + 0.5f; // center in the middle of the pixel.
    float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

    // Invert Y for DirectX-style coordinates.
    screenPos.y = -screenPos.y;

    // Unproject the pixel coordinate into a ray.
    float4 world = mul(g_sceneCB.viewProjInv, float4(screenPos, 1, 1));

    world.xyz /= world.w;
    origin = g_sceneCB.cameraPosition.xyz;
    direction = normalize(world.xyz - origin * 2);
}

// Diffuse lighting calculation.
float4 CalculateDiffuseLighting(float3 hitPosition, float3 normal)
{
    float3 pixelToLight = normalize(g_sceneCB.lightPosition.xyz - hitPosition);

    // Diffuse contribution.
    float fNDotL = max(0.0f, dot(pixelToLight, normal));

    return float4(1.f, 1.f, 1.f, 1.f);
}


[shader("raygeneration")]
FUNCTION_NAME(RAYGEN_SHADER) (void)
{
    float3 rayDir;
    float3 origin;
    
    // Generate a ray for a camera pixel corresponding to an index from the dispatched 2D grid.
    GenerateCameraRay(DispatchRaysIndex().xy, origin, rayDir);

    // Trace the ray.
    // Set the ray's extents.
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = rayDir;
    // Set TMin to a non-zero small value to avoid aliasing issues due to floating - point errors.
    // TMin should be kept small to prevent missing geometry at close contact areas.
    ray.TMin = 0.001;
    ray.TMax = 10000.0;
    RayPayload payload = { float4(0, 0, 0, 0) };
    TraceRay(Scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, payload);

    // Write the raytraced color to the output texture.
    RenderTarget[DispatchRaysIndex().xy] = payload.color;
}

[shader("closesthit")]
FUNCTION_NAME(CLOSEST_HIT_SHADER) (inout RayPayload payload, in MyAttributes attr)
{
    float3 hitPosition = HitWorldPosition();

    // Get the base index of the triangle's first 16 bit index.
    uint indexSizeInBytes = 2;
    uint indicesPerTriangle = 3;
    uint triangleIndexStride = indicesPerTriangle * indexSizeInBytes;
    uint baseIndex = PrimitiveIndex() * triangleIndexStride;

    // Load up 3 16 bit indices for the triangle.
    const uint3 indices = Load3x16BitIndices(baseIndex);

    // Retrieve corresponding vertex normals for the triangle vertices.
    float3 vertexNormals[3] = { 
        Vertices[indices[0]].normal, 
        Vertices[indices[1]].normal, 
        Vertices[indices[2]].normal 
    };

    // Compute the triangle's normal.
    // This is redundant and done for illustration purposes 
    // as all the per-vertex normals are the same and match triangle's normal in this sample. 
    float3 triangleNormal = HitAttribute(vertexNormals, attr);

    float4 diffuseColor = CalculateDiffuseLighting(hitPosition, triangleNormal);
    float4 color = g_sceneCB.lightAmbientColor + diffuseColor;

    payload.color = float4(1, 1, 1, 1);
}

[shader("miss")]
FUNCTION_NAME(MISS_SHADER) (inout RayPayload payload)
{
    float4 background = float4(0.5f, 0.2f, 0.4f, 1.0f);
    payload.color = background;
}

#endif // RAYTRACING_HLSL