#define _API_DX11
#define _AP_PC
#define SSAO_KERNEL 2
#define Z_NEAR   0.1f
#define Z_FAR    250000.0f
#define _SAMPLE_LEVEL(tex, samp, uv, lod)                   tex.SampleLevel(samp, uv, lod)
#define SET_VECTOR(TYPE, VALUE) (TYPE)(VALUE)
#define HALF2(x)  SET_VECTOR(half2, x)
#define HALF3(x)  SET_VECTOR(half3, x)
#define HALF4(x)  SET_VECTOR(half4, x)
#define FLOAT2(x) SET_VECTOR(float2, x)
#define FLOAT3(x) SET_VECTOR(float3, x)
#define FLOAT4(x) SET_VECTOR(float4, x)
#define INT2(x)   SET_VECTOR(int2, x)
#define INT3(x)   SET_VECTOR(int3, x)
#define INT4(x)   SET_VECTOR(int4, x)
#define UINT2(x)  SET_VECTOR(uint2, x)
#define UINT3(x)  SET_VECTOR(uint3, x)
#define UINT4(x)  SET_VECTOR(uint4, x)
//#include "si/system.hlsl"

SamplerState PS_SSAO_DEPTH_SMP : register(s0);

Texture2D PS_SSAO_DEPTH_TEX : register(t0);
Texture2D PS_SSAO_NORMALS_TEX : register(t1);
RWTexture2D<unorm float4> PS_SSAO_OUT_UAV: register(u0);

cbuffer cbPass : register(b0)
{
float4 PS_REG_SSAO_SCREEN[1];
float4 PS_REG_SSAO_PARAMS[1];
float4 PS_REG_SSAO_MV_1[1];
float4 PS_REG_SSAO_MV_2[1];
float4 PS_REG_SSAO_MV_3[1];
float4 SSAO_FRUSTUM_SCALE[1];
float4 SSAO_FRUSTUM_SCALE_FPMODEL[1];
}


static const half4 SSAO_NOISE[3][3] =
{
   {
      {1, 0.5, 0.5, 1},
      {0.883022, 0.821394, 0.178606, 0.883022,},
      {0.586824, 0.992404, 0.00759614, 0.586824,},
   },
   {
      {0.25, 0.933013, 0.0669873, 0.25,},
      {0.0301537, 0.67101, 0.32899, 0.0301537,},
      {0.0301537, 0.32899, 0.67101, 0.0301537,},
   },
   {
      {0.25, 0.0669873, 0.933013, 0.25,},
      {0.586824, 0.00759614, 0.992404, 0.586824,},
      {0.883022, 0.178606, 0.821394, 0.883022,},
   }
};

float2 z_to_w_coeffs()
{
#ifdef VID_USE_INVERTED_Z
   const float zf = Z_NEAR;
   const float zn = Z_FAR;
#else
   const float zn = Z_NEAR;
   const float zf = Z_FAR;
#endif
   const float2 k = float2(
      zf / (zf - zn),
      -zn * zf / (zf - zn));
   return k;
}

float z_to_w(float z)
{
   float2 k = z_to_w_coeffs();
   return k.y / (z - k.x);
}

// ----------------------------------------------------------------------------
float sample_z_lod(Texture2D t, SamplerState s, float2 tex)
{
   float4 depth = _SAMPLE_LEVEL(t, s, tex, 0);
   return depth.x;
}

// ---------------------------------------------------------------------------- 
float fp_model_cancel_z_scale(float scaledZ)
{
   scaledZ = scaledZ * 1.f; //COMMON_FPMODEL_ZSCALE[0].x;
#ifdef VID_USE_INVERTED_Z
   scaledZ = scaledZ + 0.f; // COMMON_FPMODEL_ZSCALE[0].y;
#endif

   return scaledZ;
}

// ----------------------------------------------------------------------------
float fp_model_sample_z_lod(Texture2D t, SamplerState s, float2 tex)
{
   return fp_model_cancel_z_scale(sample_z_lod(t, s, tex));
}

float sample_w_lod(Texture2D t, SamplerState s, float2 tex)
{
   return z_to_w(sample_z_lod(t, s, tex));
}

float sampleDepth(in float2 uv, in bool useFPModel)
{
   if (useFPModel)
   {
      float z = fp_model_sample_z_lod(PS_SSAO_DEPTH_TEX, PS_SSAO_DEPTH_SMP, uv);
      return z_to_w(z);
   }
   else
   {
      return sample_w_lod(PS_SSAO_DEPTH_TEX, PS_SSAO_DEPTH_SMP, uv);
   }
}


half4 SSAOFunc(float2 pixelCoord, float2 maskUV, float4 frustumScale, bool useFPModel)
{
   // constants
   float nearPlane = Z_NEAR;
   float farPlane = Z_FAR;

   // define kernel
   const half fStep = 7.f / 8.f;
   half n = 0.0;
   const half fScale = 0.025f;
   const half3 arrKernelConst[8] =
   {
#if SSAO_KERNEL == 1
         normalize(half3(1, 1, 1)) * fScale * (n += fStep),// 1
         normalize(half3(-1,-1,.5)) * fScale * (n += fStep),// 1
         normalize(half3(-1, 1,.5)) * fScale * (n += fStep),// 3
         normalize(half3(-1, 1, 1)) * fScale * (n += fStep),// 4
         normalize(half3(1,-1,.5)) * fScale * (n += fStep),// 4
         normalize(half3(-1,-1, 1)) * fScale * (n += fStep),// 2
         normalize(half3(1,-1, 1)) * fScale * (n += fStep),// 3
         normalize(half3(1, 1,.5)) * fScale * (n += fStep),// 2
#elif SSAO_KERNEL == 2
      normalize(half3(1, -1, .5)) * 0.1,
      normalize(half3(-1, -1, .5)) * 0.09,
      normalize(half3(1, 1, .5)) * 0.08,
      normalize(half3(-1, 1, .5)) * 0.07,
      normalize(half3(-1, 0, 1)) * 0.06,
      normalize(half3(1, 0, 1)) * 0.05,
      normalize(half3(0, -1, 1)) * 0.045,
      normalize(half3(0, 1, 1)) * 0.04,
#endif
   };

   //float2 uv = input.texCoord.xy;
   //float2 uvDither = input.texcoord.zw;
   //float3 viewVec = input.viewVec;
   float2 uv = maskUV;
   float2 uvDither = pixelCoord * (1.f / 3.f);
   float3 viewVec = float3(uv, 1.0) - float3(0.5, 0.5, 0.0);
   float linDepth = sampleDepth(uv, useFPModel);
   float2 perspK;
   viewVec.xy *= frustumScale.xy;
   perspK = frustumScale.zw;

   half3 arrKernel[8];

   half3 N = PS_SSAO_NORMALS_TEX.SampleLevel(PS_SSAO_DEPTH_SMP, uv, 0).xyz; //SAMPLE_LEVEL(SSAO_NORMALS, uv, 0).xyz;
   N = normalize(N * (2.0f) - (1.0f));
   N.xyz = half3(dot(N.xyz, PS_REG_SSAO_MV_1[0].xyz),
                 dot(N.xyz, PS_REG_SSAO_MV_2[0].xyz),
                 dot(N.xyz, PS_REG_SSAO_MV_3[0].xyz));

   // create random rot matrix
   half4 rotSample = SSAO_NOISE[int(pixelCoord.y) % 3][int(pixelCoord.x) % 3].xyzw * 2.0 - 1.0;
   //SAMPLE_LEVEL(SSAO_NOISE, uvDither, 0).xyzw * 2.0 - 1.0;

   for (int i = 0; i < 8; i++)
   {
      arrKernel[i].z = arrKernelConst[i].z;
      arrKernel[i].xy = float2(dot(arrKernelConst[i].xy, rotSample.xy), dot(arrKernelConst[i].xy, rotSample.zw));
      arrKernel[i] = reflect(arrKernel[i], normalize(N + half3(0, 0, -1.0f)));
   }

   half3 vSampleScale = PS_REG_SSAO_PARAMS[0].xxx;
   if (useFPModel)
   {
      vSampleScale *= 0.2f;
   }
   else
   {
      // make area bigger if distance more than 32 meters
      vSampleScale *= (

         (1.4f) + linDepth /

         (10.f)
      );
      // clamp area at close distance (< 2M)
      vSampleScale *= saturate(linDepth * (0.5f));
   }

   float fDepthRangeScale = PS_REG_SSAO_PARAMS[0].z / vSampleScale.z;

   float fDepthTestSoftness = 64.f / vSampleScale.z;


   float3 eyePosition = linDepth * viewVec / viewVec.z;

   // sample
   half4 vSkyAccess = HALF4(0.0f);
   half3 vIrrSample;
   half4 vDistance;
   float4 fRangeIsValid;
   float sum = 0.0f; //sum of weights
   float3 bentNormal = FLOAT3(0.f);

   float fHQScale = 0.5f; //scale of additional samples
   float unoccludedDirections = 0.f;

   for (int i = 0; i < 2; i++)
   {
      vIrrSample = arrKernel[i * 4 + 0] * vSampleScale + eyePosition;
      vIrrSample.xy /= vIrrSample.z;
      vIrrSample.xy *= perspK;
      vIrrSample.xy += half2(0.5, 0.5);
      vDistance.x = sampleDepth(vIrrSample.xy, useFPModel) - vIrrSample.z;

      vIrrSample = arrKernel[i * 4 + 1] * vSampleScale + eyePosition;
      vIrrSample.xy /= vIrrSample.z;
      vIrrSample.xy *= perspK;
      vIrrSample.xy += half2(0.5, 0.5);
      vDistance.y = sampleDepth(vIrrSample.xy, useFPModel) - vIrrSample.z;

      vIrrSample = arrKernel[i * 4 + 2] * vSampleScale + eyePosition;
      vIrrSample.xy /= vIrrSample.z;
      vIrrSample.xy *= perspK;
      vIrrSample.xy += half2(0.5, 0.5);
      vDistance.z = sampleDepth(vIrrSample.xy, useFPModel) - vIrrSample.z;

      vIrrSample = arrKernel[i * 4 + 3] * vSampleScale + eyePosition;
      vIrrSample.xy /= vIrrSample.z;
      vIrrSample.xy *= perspK;
      vIrrSample.xy += half2(0.5, 0.5);
      vDistance.w = sampleDepth(vIrrSample.xy, useFPModel) - vIrrSample.z;

      float4 vDistanceScaled = vDistance * fDepthRangeScale;
      fRangeIsValid = 1 - (saturate(-vDistanceScaled) + saturate(abs(vDistanceScaled))) * 0.5;

      float4 dist = lerp(HALF4(1.0f), saturate(vDistance * fDepthTestSoftness), saturate(fRangeIsValid * 2.0)) * (max(
         fRangeIsValid, 0.5));

      vSkyAccess += dist;

      sum += dot(max(fRangeIsValid, 0.5), FLOAT4(1.0));

      bentNormal += normalize(arrKernel[i * 4 + 0]) * dist.x + normalize(arrKernel[i * 4 + 1]) * dist.y +
         normalize(arrKernel[i * 4 + 2]) * dist.z + normalize(arrKernel[i * 4 + 3]) * dist.w;
      unoccludedDirections += dot(dist, float4(1.f, 1.f, 1.f, 1.f));
   }

   float res = saturate(dot(vSkyAccess, HALF4(1.0 / sum)));
   res = sqrt(res); // sqrt is symmetric to square in SSAOGet()

   bentNormal = normalize(bentNormal / unoccludedDirections);

   bentNormal = (
      bentNormal.x * PS_REG_SSAO_MV_1[0].xyz +
      bentNormal.y * PS_REG_SSAO_MV_2[0].xyz +
      bentNormal.z * PS_REG_SSAO_MV_3[0].xyz) * 0.5f + 0.5f;

   return float4(bentNormal.x, bentNormal.y, bentNormal.z, res);
}


[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
   uint2 coord = DTid.xy;
   float2 pixelCoord = coord + float2(0.5f, 0.5f);
   float2 maskUV = pixelCoord * PS_REG_SSAO_SCREEN[0].xy;

   if (maskUV.x < 1 && maskUV.y < 1)
   {
      half4 color = half4(1, 1, 1, 1);
      float depth = PS_SSAO_DEPTH_TEX.Load(int3(pixelCoord, 0)).x;
      //float depth = LOAD_2D_FLOAT(SSAO_DEPTH, pixelCoord).x;
      if (depth >= PS_REG_SSAO_PARAMS[0].w)
      {
         color = SSAOFunc(pixelCoord, maskUV, SSAO_FRUSTUM_SCALE_FPMODEL[0], true);
      }
      else
      {
         color = SSAOFunc(pixelCoord, maskUV, SSAO_FRUSTUM_SCALE[0], false);
      }
      PS_SSAO_OUT_UAV[coord] = color;
   }
}
