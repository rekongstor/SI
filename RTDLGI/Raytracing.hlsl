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
#include "Common.hlsl"

// Global root signature
RWTexture3D<float> RenderTarget : register(u0);
RWTexture2D<float> RenderTargetOut : register(u1);
RWTexture2D<float> RenderTargetOutDist : register(u2);
RWBuffer<uint> DLGIinputs : register(u3);
RWBuffer<uint> PosInputs : register(u4);

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


inline void GenerateGIRay(uint3 index, out float3 origin, out float3 direction)
{
   direction = normalize(g_sceneCB.lightDirection.xyz);
   origin = (index + 0.5f) / float(GI_RESOLUTION) * 2.f - 1.f;
}


inline void GenerateDistRay(uint3 index, out float3 origin, out float3 direction)
{
   direction = normalize(float3(rand_1_05(index.xy), rand_1_05(index.yz), rand_1_05(index.zx)));
   origin = (index + 0.5f) / float(GI_RESOLUTION) * 2.f - 1.f;
}


[shader("raygeneration")]
FUNCTION_NAME(RAYGEN_SHADER) (void)
{
   uint3 dr = DispatchRaysIndex().xyz;
   if (DispatchRaysDimensions().x == GI_RESOLUTION)
   {
      float3 rayDir;
      float3 origin;
      GenerateGIRay(dr, origin, rayDir);

      RayDesc ray;
      ray.Origin = origin;
      ray.Direction = rayDir;
      ray.TMin = 0.0001;
      ray.TMax = 2;
      RayPayload payload = { float4(0, 0, 0, 0) };
      TraceRay(Scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, payload);

      // Write the raytraced color to the output texture.
      RenderTarget[dr.xyz] = payload.color.x;
      RenderTargetOut[uint2(dr.x * GI_RESOLUTION + dr.y, dr.z + (int)round(max(g_sceneCB.counter.x, 0.f) * GI_RESOLUTION))] = payload.color.x;
      PosInputs[(dr.x * GI_RESOLUTION + dr.y + dr.z * GI_RESOLUTION * GI_RESOLUTION) * 3 + 0] = f32tof16(float(dr.x) / float(GI_RESOLUTION) * 2.f - 1.f);
      PosInputs[(dr.x * GI_RESOLUTION + dr.y + dr.z * GI_RESOLUTION * GI_RESOLUTION) * 3 + 1] = f32tof16(float(dr.y) / float(GI_RESOLUTION) * 2.f - 1.f);
      PosInputs[(dr.x * GI_RESOLUTION + dr.y + dr.z * GI_RESOLUTION * GI_RESOLUTION) * 3 + 2] = f32tof16(float(dr.z) / float(GI_RESOLUTION) * 2.f - 1.f);
   }
   else
   {
      float3 rayDir;
      float3 origin;
      GenerateDistRay(dr, origin, rayDir);

      RayDesc ray;
      ray.Origin = origin;
      ray.Direction = rayDir;
      ray.TMin = 0.0001;
      ray.TMax = 2;
      RayPayload payload = { float4(0, 0, 0, 0) };
      TraceRay(Scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, payload);

      RenderTargetOutDist[uint2(dr.x * RAYS_PER_AXIS + dr.y, dr.z + (int)round(max(g_sceneCB.counter.x, 0.f) * RAYS_PER_AXIS))] = (ray.TMax - payload.color.w) / ray.TMax;
      if ((int)max(round(g_sceneCB.counter.x), 0.f) == 0)
         DLGIinputs[dr.x * RAYS_PER_AXIS + dr.y + dr.z * RAYS_PER_AXIS * RAYS_PER_AXIS] = f32tof16((ray.TMax - payload.color.w) / ray.TMax * 2.f - 1.f);
   }
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

   payload.color.xyz = float3(0, 0, 0);
   payload.color.w = RayTCurrent();
}

[shader("miss")]
FUNCTION_NAME(MISS_SHADER) (inout RayPayload payload)
{
   payload.color.xyz = float3(1, 1, 1);
   payload.color.w = RayTCurrent();
}

#endif // RAYTRACING_HLSL