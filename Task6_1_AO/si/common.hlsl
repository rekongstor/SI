#ifndef _COMMON_PIXEL_SHADER_STUFF_INCLUDED_
#define _COMMON_PIXEL_SHADER_STUFF_INCLUDED_ 1

#include "system.hlsl"
#include "registers_common.hlsl"
//#include "common_funcs.hlsl"
//#include "common_math.hlsl"

#define AMBIENT_MAX_LUMINANCE 64.0f
#define MAX_SPECULAR_POWER 2048.0f  // exp2(GLOSS_SCALE + GLOSS_BIAS)
//#define MATH_FP_HALF_MAX 65504.f // 5e10m
#define MATH_FP_HALF_MAX 64512.f // 5e5m
//#define MATH_FP_HALF_MAX 64845.f // 5e6m

#define Z_NEAR   0.1f
#define Z_FAR    250000.0f

#define SCREEN_WIDTH_RCP    COMMON_VP_PARAMS[0].x
#define SCREEN_HEIGHT_RCP   COMMON_VP_PARAMS[0].y
#define SCREEN_SIZE_RCP     COMMON_VP_PARAMS[0].xy  
#define SCREEN_WIDTH        (1.f / SCREEN_WIDTH_RCP)
#define SCREEN_HEIGHT       (1.f / SCREEN_HEIGHT_RCP)

#if defined(COMMON_VP_FPS_MODEL)
   #define ZSCALE_CORRECTION_X (PS_REG_COMMON_WPN_ZSCALE[0].x)
   #define ZSCALE_CORRECTION_Y (PS_REG_COMMON_WPN_ZSCALE[0].y)
#else
   #define ZSCALE_CORRECTION_X 1
   #define ZSCALE_CORRECTION_Y 0
#endif

// DS: this is deprecated, use _SAMPLE instead
half4 TEX2D_HALF(Texture2D t, SamplerState s, float2 uv)         { return half4(_SAMPLE(t,s,uv)); }

// Centroid interpolation modifier - applicable only to xbox
#define CENTROID(interp) interp

#define EXP_BASE     half(1.02f)
#define EXP_OFFSET   half(64.f)

#if defined(_DEBUG)
   static int   gDebugDynLightsCount = 0;
   static int   gDebugReflectionsCount = 0;
   static float gDebugMipLevel = 0;
#endif


//float3 calcScrUVAndDepthMirror(float3 wpos)
//{
//   float4 posProj = apply_view_proj_matrix_world(float4(wpos, 1));
//   float2 uv = posProj.xy / posProj.w;
//   uv *= float2(0.5, -0.5);
//   uv += 0.5;
//   uv = fmod(uv, float2(2, 2));
//   uv = 1 - abs(abs(uv) - 1);
//   return float3(uv, posProj.w);
//}

float2 unpack_normal (in float2 n)
{
   return float2(n * (255.0f / 127.f) - (128.0f / 127.f));
}

float3 unpack_normal (in float3 n)
{
   return float3(n * (255.0f / 127.f) - (128.0f / 127.f));
}

// ----------------------------------------------------------------------------
//half4 HDR2RGBe(half3 hdr)
//{
//   half max_ch = max(hdr.x, max(hdr.y, hdr.z));
//   half fexp = log2(max_ch) / log2(EXP_BASE);
//   half4 res;
//   res.w = saturate((fexp + EXP_OFFSET) / half(256.f) );
//   half s = res.w * half(256.f) - EXP_OFFSET;
//   res.xyz = hdr.xyz / exp2(s * log2(EXP_BASE));
//   //pow(EXP_BASE, res.w * 256.h - EXP_OFFSET);
//   return res;
//}
//
//// ----------------------------------------------------------------------------
//half3 RGBe2HDR(half4 rgbe)
//{
////   half s = pow(EXP_BASE, rgbe.w * 256.h - EXP_OFFSET);
//   half s = rgbe.w * half(256.f) - EXP_OFFSET;
//   s = exp2(s * log2(EXP_BASE));
//   return rgbe.xyz * s;
//}


// ----------------------------------------------------------------------------
float3 hsv2rgb(float3 c)
{
   const float4 K = float4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
   float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
   return c.z * lerp(K.xxx, saturate(p - K.xxx), c.y);
}

// ----------------------------------------------------------------------------
float3 rgb2hsv(float3 c)
{
   const float EPS = 1.0e-10;
   const float4 K = float4(0.f, -1.f / 3.f, 2.f / 3.f, -1.f);
   float4 p = c.g < c.b ? float4(c.bg, K.wz) : float4(c.gb, K.xy);
   float4 q = c.r < p.x ? float4(p.xyw, c.r) : float4(c.r, p.yzx);
    
   float d = q.x - min(q.w, q.y);
   return float3(abs(q.z + (q.w - q.y) / (6.0 * d + EPS)), d / (q.x + EPS), q.x);
}


// ----------------------------------------------------------------------------
float3 rgb2yuv(float3 rgb) 
{
   return float3(
      dot(rgb, float3(0.299f, 0.587f, 0.114f)),
      dot(rgb, float3(-0.1687f, -0.3313f, 0.5f)) + .5f,
      dot(rgb, float3(0.5f, -0.4187f, -0.0813f)) + .5f
   );
}

// ----------------------------------------------------------------------------
float3 yuv2rgb(float3 yuv)
{
   float Y = yuv.x;
   float b = yuv.y - .5f;
   float r = yuv.z - .5f;
   
   return float3(
       Y + 1.402f * r,
       Y - 0.34414f * b - 0.71414f * r,
       Y + 1.772f * b
   );
}

// ----------------------------------------------------------------------------
float2 reflect_2D (in float2 i, in float2 n)
{   
   return reflect(i, n);
}

// ----------------------------------------------------------------------------
half2 reflect_2D_h (in half2 i, in half2 n)
{   
   return half2(reflect(i, n));
}

// ----------------------------------------------------------------------------
half pow8_h(in half x)
{
   half r = x*x;//2
   r *= r;//4
   r *= r;//8
   return r;
}

// ----------------------------------------------------------------------------
float3 ConvertSRGBToLinear(float3 color)
{
   float3 mask = saturate(sign(color - 0.04045f));
   return (1-mask) * color / 12.92f + mask * pow((color + 0.055f)/(1 + 0.055f), FLOAT3(2.4f));
}

// ----------------------------------------------------------------------------
float3 ConvertLinearToSRGB(float3 color)
{
   return VECTOR_SELECT(LESS_THAN(color, float3(0.0031308f, 0.0031308f, 0.0031308f)) ,
      12.92f * color ,
      1.055f * pow(color, FLOAT3(1.f / 2.4f)) - 0.055f);
}

// ----------------------------------------------------------------------------
// Color gradient of 0 - 1 range
float3 ThermoGrad(float x)
{
   float3 res = ZERO_OUT(float3);
   res = lerp(res, float3(1.f, 0.f, 0.f), saturate(x * 7));    // red    = 0.14
   res = lerp(res, float3(1.f, 1.f, 0.f), saturate(x * 7 - 1));// yellow = 0.29
   res = lerp(res, float3(0.f, 1.f, 0.f), saturate(x * 7 - 2));// green  = 0.43
   res = lerp(res, float3(0.f, 1.f, 1.f), saturate(x * 7 - 3));// cyan   = 0.57
   res = lerp(res, float3(0.f, 0.f, 1.f), saturate(x * 7 - 4));// blue   = 0.71
   res = lerp(res, float3(1.f, 0.f, 1.f), saturate(x * 7 - 5));// pink   = 0.86
   res = lerp(res, float3(1.f, 1.f, 1.f), saturate(x * 7 - 6));// white  = 1.00
   return res;
}


// ----------------------------------------------------------------------------   
//
// NOTE: 'fetch_xxx' methods VS '_SAMPLE_xxx' macroses
//
// Use 'fetch_xxx' methods for all ordinary 'logical' lookups (i.e. typical albedo, 
// normals, spec/gloss lookups) in all shaders. These methods are subject to 
// some global corrections (like normals zero-biasing or global gradients disable 
// switch). These methods should be mostly used by 'scene objects' shaders.
//
// Use _SAMPLE_xxx macros if you want just specific low-level lookup which won't
// be changed/filtered in any way. These methods should be mostly used for 
// post-processing/auxiliary shaders.
//
// ----------------------------------------------------------------------------   

// ----------------------------------------------------------------------------   
half4 fetch_color(Texture2D t, SamplerState s, float2 uv)
{
   return _SAMPLE(t, s, uv);
}

half4 fetch_color_lod(Texture2D t, SamplerState s, float2 uv, float lod)
{
   return _SAMPLE_LEVEL(t, s, uv, lod);
}

// ----------------------------------------------------------------------------   
half4 fetch_color_grad(Texture2D t, SamplerState s, float2 uv, float4 grad)
{
#if !defined(_DISABLE_GRAD_LOOKUPS)
   return _SAMPLE_GRAD(t, s, uv.xy, grad.xy, grad.zw);
#else
   return _SAMPLE(t, s, uv.xy);
#endif
}
            

#define DBG_F_ALBEDO_CORRECTION_MIP  int(PS_REG_COMMON_DEBUG_SHOW_LIGHTING[3].z)
#define ALB_COR_ENABLED              (PS_REG_COMMON_DEBUG_SHOW_LIGHTING[3].x > 0.0f)

// ---------------------------------------------------------------------------- 
// Realtime albedo texture brightness tweaking (for quick visual debug)
// ---------------------------------------------------------------------------- 
//half4 dbg_apply_albedo_correction(
//   in half4 color,      // albedo tex value to be corrected (sampled as usual)
//   in half4 mip_color   // albedo tex value sampled from DBG_F_ALBEDO_CORRECTION_MIP level (used as avg lum for the whole texture)
//   )
//{
//   float k = 1.f;
//
//   bool  ALB_COR_SHOW_MIP  = PS_REG_COMMON_DEBUG_SHOW_LIGHTING[3].x > 1.0f;
//   float ALB_COR_LUM       = PS_REG_COMMON_DEBUG_SHOW_LIGHTING[3].y;
//   float ALB_COR_AMOUNT    = PS_REG_COMMON_DEBUG_SHOW_LIGHTING[3].w;
//   
//   // float avgLum = (mip_color.x + mip_color.y + mip_color.z) / 3.f;
//   float avgLum = luminance(mip_color.xyz);
//   float targetLum = lerp(avgLum, ALB_COR_LUM, ALB_COR_AMOUNT);
//   k = targetLum / (avgLum + 0.01f);
//   
//   if (ALB_COR_SHOW_MIP) {
//      return float4(saturate(mip_color.xyz * k), color.w);
//   }
//   
//   color.xyz = saturate(color.xyz * k);
//   return color;
//}


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
float fp_model_cancel_z_scale_in_clip_space(float scaledZ, float w)
{  
   scaledZ = scaledZ * 1.f; // COMMON_FPMODEL_ZSCALE[0].x;
   #ifdef VID_USE_INVERTED_Z
   scaledZ = scaledZ + 0.f; // COMMON_FPMODEL_ZSCALE[0].y* w;
   #endif
   
   return scaledZ;
}

                                                                                 
// ---------------------------------------------------------------------------- 
half4 fetch_color_grad_albedo(Texture2D t, SamplerState s, float2 uv, float4 grad)
{
   float4 color = fetch_color_grad(t, s, uv, grad);
   
   #if defined(_DEBUG)
      if (ALB_COR_ENABLED) {
         half4 mip_color = _SAMPLE_LEVEL(t, s, uv.xy, DBG_F_ALBEDO_CORRECTION_MIP);
         return dbg_apply_albedo_correction(color, mip_color);
      }
   #endif

   return color;
}


// ----------------------------------------------------------------------------
// Restore Z component of unit vector
// ----------------------------------------------------------------------------
float get_normal_z(float2 nxy)
{
   // DS: 0.00001f - zero normal fix
   // Think about the case:
   // - identity normal length (dot(xy,xy)==1)
   // - get_normal_z(xy) will give you exactly 0
   // - zero xy scale applied somewhere in detail mapping
   // - xyz == 0 at the end
   return sqrt(1.00001f - saturate(dot(nxy, nxy))); 
}



// ----------------------------------------------------------------------------
half4 fetch_cube(TextureCube t, SamplerState s, float3 uv)
{
   return _SAMPLE_CUBE(t, s, uv);
}

// ----------------------------------------------------------------------------
float4 fetch_cube_lod(TextureCube t, SamplerState s, float3 uv, float lod)
{
   return _SAMPLE_CUBE_LEVEL(t, s, uv.xyz, lod);
}   

// ----------------------------------------------------------------------------
float2 fetch_double_mask(Texture2D t, SamplerState s, float2 uv)
{
   return fetch_color(t, s, uv).xy;
}

// ----------------------------------------------------------------------------
half2 fetch_double_mask_grad(Texture2D t, SamplerState s, float2 uv, float4 grad)
{
   return fetch_color_grad(t, s, uv, grad).xy;
}

// ----------------------------------------------------------------------------
half fetch_single_mask(Texture2D t, SamplerState s, float2 uv)
{
   return fetch_color(t, s, uv).x;
}

// ----------------------------------------------------------------------------
half fetch_single_mask_grad(Texture2D t, SamplerState s, float2 uv, float4 grad)
{
   return fetch_color_grad(t, s, uv, grad).x;
}


// ----------------------------------------------------------------------------
float2 fetch_norm_xy(Texture2D t, SamplerState s, float2 uv)
{
   float2 n = fetch_double_mask(t, s, uv);
   n = unpack_normal(n);
   return n;
}

// ----------------------------------------------------------------------------
half4 fetch_norm_xyzw(Texture2D t, SamplerState s, float2 uv)
{
   half4 n = fetch_color(t, s, uv);
   n.xyz = unpack_normal(n.xyz);
   return n;
}

// ----------------------------------------------------------------------------
half2 fetch_norm_xy_lod(Texture2D t, SamplerState s, float2 uv, float lod)
{
   half2 n = _SAMPLE_LEVEL(t, s, uv, lod).xy;
   n = unpack_normal(n);
   return n;
}

// ----------------------------------------------------------------------------
half2 fetch_norm_xy_grad(Texture2D t, SamplerState s, float2 uv, float4 grad)
{
   half2 n = fetch_double_mask_grad(t, s, uv, grad);
   n = unpack_normal(n);
   return n;
}


// ----------------------------------------------------------------------------
float3 fetch_norm(Texture2D t, SamplerState s, float2 uv)
{
   float3 n;
   n.xy = fetch_norm_xy(t, s, uv);
   n.z = get_normal_z(n.xy);
   return n;
}


// ----------------------------------------------------------------------------
float3 fetch_norm_lod(Texture2D t, SamplerState s, float2 uv, float lod)
{
   float3 n;
   n.xy = fetch_norm_xy_lod(t, s, uv, lod);
   n.z = get_normal_z(n.xy);
   return n;
}


// ----------------------------------------------------------------------------
// Convert Z (in [0, 1] range) to W (in [near_plane, far_plane] range)
// ----------------------------------------------------------------------------
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
float2 z_to_w(float2 z)
{
   float2 k = z_to_w_coeffs();
   return k.yy / (z - k.xx);
}
float4 z_to_w(float4 z)
{
   float2 k = z_to_w_coeffs();
   return k.yyyy / (z - k.xxxx);
}

float z_to_w(float z, float2 k)
{
   return k.y / (z - k.x);
}

// ----------------------------------------------------------------------------
// Convert W (in [near_plane, far_plane] range) to Z (in [0,1] range)
// ----------------------------------------------------------------------------
float w_to_z (float w)
{
#ifdef VID_USE_INVERTED_Z
   const float zf = Z_NEAR;
   const float zn = Z_FAR;
#else
   const float zn = Z_NEAR;
   const float zf = Z_FAR;
#endif
   const float k1 = zf / (zf - zn);
   const float k2 = -zn * k1;
   return k2 / w + k1;//w * k1 + k2;
}

float4 w_to_z (float4 w)
{
#ifdef VID_USE_INVERTED_Z
   const float zf = Z_NEAR;
   const float zn = Z_FAR;
#else
   const float zn = Z_NEAR;
   const float zf = Z_FAR;
#endif
   const float k1 = zf / (zf - zn);
   const float k2 = -zn * k1;
   return k2 / w + k1;
}

float w_to_z (float w, float2 k)
{
   return k.y / w + k.x;
}

// ----------------------------------------------------------------------------
// Convert Depth from ARGB for _Z_SAMPLING_DOT textures, pass through otherwize
// ----------------------------------------------------------------------------
float depth_component_approximate(float4 z_sample_data)
{
   return z_sample_data.r;
}
// ----------------------------------------------------------------------------
// high precision version of depth_component 
// ----------------------------------------------------------------------------
float depth_component_hp(float4 z_sample_data)
{
#ifdef VID_USE_INVERTED_Z
   float3 depthColor = floor(z_sample_data.arg * 255.15f);
   float m = depthColor.g * 256.0f + depthColor.b;
   m = m / 65535.0f;

   return ldexp(1.0f + m, depthColor.r - 127.0f);
#else
   //float3 kk = float3(0.996093809371817670572857294849, 0.0038909914428586627756752238080039, 1.5199185323666651467481343000015e-5) / 255.0f;
   float3 kk = float3(0.0039062502328306575, 1.5258789971994756e-5, 5.9604648328104516e-8);
   float3 vTex = floor(z_sample_data.arg * 255.0f + 0.5f);
   return dot(kk, vTex);
#endif
}


// ----------------------------------------------------------------------------
// Get Z value from z-buffer texture (in [0, 1] range)
// ----------------------------------------------------------------------------
float sample_z(Texture2D t, SamplerState s, float2 tex)
{
   float4 depth = _SAMPLE(t, s, tex);
   return depth.x;
}

// ----------------------------------------------------------------------------
float get_w(float depthValue)
{
   return z_to_w(depthValue);
}

// ----------------------------------------------------------------------------
float sample_z_lod(Texture2D t, SamplerState s, float2 tex)
{
   float4 depth = _SAMPLE_LEVEL(t, s, tex, 0);
   return depth.x;
}

// ----------------------------------------------------------------------------
float fp_model_sample_z_lod(Texture2D t, SamplerState s, float2 tex)
{
   return fp_model_cancel_z_scale(sample_z_lod(t, s, tex));
}


// ----------------------------------------------------------------------------
// Get W value from z-buffer texture, range [near_plane, far_plane]
// ----------------------------------------------------------------------------
float sample_w(Texture2D t, SamplerState s, float2 tex)
{
   return z_to_w(sample_z(t, s, tex));
}

float sample_w_lod(Texture2D t, SamplerState s, float2 tex)
{
   return z_to_w(sample_z_lod(t, s, tex));
}

// ----------------------------------------------------------------------------

// screenTexcoord - screen tc in range 0..1
//float3 GetViewsPosFromScreenTexCoord (Texture2D t, SamplerState s, in float2 scrUv)
//{
//   return float3((scrUv.x * 2.f - 1.f) / COMMON_PROJ_MATRIX[0][0], ((1.f - scrUv.y) * 2.f - 1.f) / COMMON_PROJ_MATRIX[1][1], 1.f) * sample_w(t, s, scrUv);
//}
//
//// ----------------------------------------------------------------------------
//// Generate screen mask tex coord
//// ----------------------------------------------------------------------------
//float2 GenerateScreenMaskTexCoord(in float2 vPos)
//{
//   return vPos * COMMON_VP_PARAMS[0].xy;
//}

// ----------------------------------------------------------------------------
// 
// ----------------------------------------------------------------------------
float2 AdjustScreenMaskUV_MSAA(in float2 maskUV, in float center_w, in float centroid_w, in SamplerState samp_edge_mask)
{
   return maskUV;
}
 

float2 GetPixelAlignedUV(float2 uv, float2 texSize)
{
   float2 texPose = uv * texSize + 0.5;
   float2 ipos    = floor(texPose);
   float2 fpos    = frac(texPose);
   texPose        = ipos + fpos * fpos * fpos * (fpos * (fpos * 6.0f - 15.0f) + 10.0f);
   uv             = (texPose - 0.5f) / texSize;
   return uv;
}

// ----------------------------------------------------------------------------
// 
// ----------------------------------------------------------------------------
void FetchLightbuffer(out half3 diff, out half3 spec, in Texture2D tex_col, in SamplerState samp_col, in float2 uv)
{
   diff = TEX2D_HALF(tex_col, samp_col, uv * float2(1.f, 0.5f)).xyz;
   spec = TEX2D_HALF(tex_col, samp_col, uv * float2(1.f, 0.5f) + float2(0.f, 0.5f)).xyz;
}
   
   
// ----------------------------------------------------------------------------
// 
// ----------------------------------------------------------------------------
half pow_fast(half k, half2 reg) 
{
   k = saturate(k *reg.x + reg.y);
   k *= k; 
   k *= k;
   return k;
}
   

// ----------------------------------------------------------------------------
// 
// ----------------------------------------------------------------------------
float StereoAdjustPosX(in float posx_view, in float depth_view, in Texture2D tex, in SamplerState samp, in float4 psReg)
{
#if defined(_AP_PC) && defined(_NV_HW)
// TODO: Texture1D or Texture2D
   float eye_sign = tex.Sample(samp, 0.0625f).x;
   
   float convergence = psReg.x;
   float separation  = psReg.y;
    
   posx_view = posx_view + eye_sign * separation * (depth_view - convergence);
#endif
   return posx_view;
}


// ----------------------------------------------------------------------------
// Convert projection space point from current frame to previous frame
// in UV space (same as clip space, but x,y in [0;1] instead of [-1;1])
//float3 ConvertPointPrevFrame(float3 viewPos)
//{
//   float3 res;
//   res.z   = 1.0f;
//   res.xy  = (viewPos.xy * float2(2.0, -2.0) + float2(-1.0f, 1.0f));
//   res *= viewPos.z;
//
//   res = float3(
//      dot(float4(res, 1), PS_REG_REFLECTIONS_MATRVIEW_PREV[0]),
//      dot(float4(res, 1), PS_REG_REFLECTIONS_MATRVIEW_PREV[1]),
//      dot(float4(res, 1), PS_REG_REFLECTIONS_MATRVIEW_PREV[2])
//   );
//   
//   res.xy = res.xy / res.z;
//   res.xy *= float2(0.5f, -0.5f);// TODO: move this to matrix transformation
//   res.xy += float2(0.5f, 0.5f);
//   return res;
//}


// ----------------------------------------------------------------------------
// Convert projection space point from current frame to previous frame
// in clip (projection) space
//float3 ConvertPointPrevFrameClip(float3 viewPos)
//{
//   float3 res;
//   res.z = viewPos.z;
//   res.xy = viewPos.xy * viewPos.z;
//
//   res = float3(
//      dot(float4(res, 1), PS_REG_REFLECTIONS_MATRVIEW_PREV[0]),
//      dot(float4(res, 1), PS_REG_REFLECTIONS_MATRVIEW_PREV[1]),
//      dot(float4(res, 1), PS_REG_REFLECTIONS_MATRVIEW_PREV[2])
//   );
//   
//   res.xy = res.xy / res.z;
//   return res;
//}


// ----------------------------------------------------------------------------
float3 TAATonemap(float3 col) {
   float luma = dot(col, float3(0.2f, 0.6f, 0.2f));
   return col / (1 + luma);
}


// ----------------------------------------------------------------------------
float3 TAADetonemap(float3 col) {
   float luma = dot(col, float3(0.2f, 0.6f, 0.2f));
   return col / (1 - luma);
}


// ----------------------------------------------------------------------------
// 
// ----------------------------------------------------------------------------
//half3 global_ambient(in half3 wnorm)
//{
//   return PS_REG_COMMON_AMBIENT[0].xyz;
//}


// ----------------------------------------------------------------------------
// 
// ----------------------------------------------------------------------------
half3 GetLMDataHDR(in Texture2D tex, in SamplerState samp, in half2 uv)
{
   return _SAMPLE(tex, samp, uv).xyz;
}

// ----------------------------------------------------------------------------
half blend_hardlight(half back, half front)
{
   return (front < .5f) ?  
      (2 * front * back) :
      1 - (2 * (1 - back) * (1 - front));
}

// ----------------------------------------------------------------------------
half3 blend_hardlight(half3 back, half3 front)
{
   return VECTOR_SELECT(LESS_THAN(front, half3(.5f, .5f, .5f)),  
      (2 * front * back) ,
      1 - (2 * (1 - back) * (1 - front)));
}

// ----------------------------------------------------------------------------
half3 blend_overlay(half3 back, half3 front)
{
   return VECTOR_SELECT(LESS_THAN(back, half3(.5f, .5f, .5f)) ,
      (2 * front * back) ,
      1 - (2 * (1 - back) * (1 - front)));
}

// ----------------------------------------------------------------------------
float4 uint2rgba(uint src)
{
   float4 dest;
   dest.b = src & 0xFF; src >>= 8;
   dest.g = src & 0xFF; src >>= 8;
   dest.r = src & 0xFF; src >>= 8;
   dest.a = src & 0xFF;
   return dest / 255.f;
}

float3 uint2rgb(uint src)
{
   float3 dest;
   dest.b = src & 0xFF; src >>= 8;
   dest.g = src & 0xFF; src >>= 8;
   dest.r = src & 0xFF;
   return dest / 255.f;
}

// ----------------------------------------------------------------------------
// WARNING: range is [0,1)

inline float4 DecodeFloatRGBA(float v)
{
   float4 enc = float4(1.0, 255.0, 65025.0, 160581375.0) * v;
   enc = frac(enc);
   enc -= enc.yzww * float4(1.0/255.0, 1.0/255.0, 1.0/255.0, 0.0);
   return enc;
}

inline float EncodeFloatRGBA(float4 rgba)
{
   return dot(rgba, float4(1.0, 1/255.0, 1/65025.0, 1/160581375.0));
}

// ----------------------------------------------------------------------------
// WARNING: range is [0,1)

inline float2 EncodeFloatRG(float v)
{
   float2 enc = float2(1.0, 255.0) * v;
   enc = frac(enc);
   enc -= enc.yy * float2(1.0/255.0, 0.0);
   return enc;
}

inline float DecodeFloatRG(float2 rgba)
{
   return dot(rgba, float2(1.0, 1/255.0));
}

// ----------------------------------------------------------------------------

float Pack2PNForFP16(float a, float b)
{
   uint uValue;
   uint uEncodedb;
   
   uValue = (uint(a * 255.0 + 0.5)) << 13;
   uEncodedb = uint(b * 239.0 + 0.5);
   uEncodedb += 0x08;
   uValue |= ((uEncodedb & 0x80) != 0) ? 0x40000000 : 0x38000000;
   
   uint uTemp = (uEncodedb & 0x7E) << 20;
   uTemp |= (uEncodedb & 0x01) << 31;
   
   uValue |= uTemp;
   
   return asfloat(uValue);
}

float2 Unpack2PNForFP16(float fFloatFromFP16)
{
   float a, b;
   uint uEncodedb;
   uint uInputFloat = asuint(fFloatFromFP16);
   
   a = float((uInputFloat >> 13) & 0xFF) / 255.0;
   uEncodedb = (uInputFloat & 0x40000000) >> 23;
   uEncodedb |= (uInputFloat & 0x07E00000) >> 20;
   uEncodedb = uEncodedb - 0x08;
   uEncodedb |= (uInputFloat & 0x80000000) >> 31;
   b = float(uEncodedb) / 239.0;
   return float2(a, b);
}

// h1, h2 - two heightmaps [0;1], [-.5;.5] - doesn't matter
// s - smooth zone, when abs(h1-h2) < s/2 - lerp
// w - weight
float HeightMapBlend(float h1, float h2, float w, float s)
{
   return saturate((h2 - h1 + w * (2 + s) - 1 - s * 0.5f) / s + 0.5f);
}

// h1 [-.5;.5]
// s - smooth zone
// w - weight
float HeightMapBlendSingle(float h, float w, float s)
{
   return saturate((h - 0.5f + w * (1 + s) - s * 0.5f) / s + 0.5f);
}

static const float4 LIGHT_COLOR_WHITE   = float4(1.0f, 1.0f, 1.0f, 1.0f);
static const float4 LIGHT_COLOR_RED     = float4(255,   0,   0, 255) / 255.f;
static const float4 LIGHT_COLOR_GREEN   = float4(  0, 255,   0, 255) / 255.f;
static const float4 LIGHT_COLOR_BLUE    = float4(  0,   0, 255, 255) / 255.f;
static const float4 LIGHT_COLOR_YELLOW  = float4(255, 255,   0, 255) / 255.f;


// ----------------------------------------------------------------------------
//bool IsUseSSRTGI (void)
//{
//   return PS_REG_COMMON_DEBUG_SHOW_LIGHTING[2].z == 1.0f;
//}
//// ----------------------------------------------------------------------------
//bool IsTestProbe (void)
//{
//   return GET_INST_DATA(OBJ_DATA_BUFFER).PS_REG_COMMON_TPL_IDX.y == 1.f; 
//}
//// ----------------------------------------------------------------------------
//int GetTestProbeVis (void)
//{
//   return asint(PS_REG_COMMON_DEBUG_SHOW_LIGHTING[5].w); 
//}
//// ----------------------------------------------------------------------------
//bool IsLightGridFilterOff (void)
//{
//   return PS_REG_COMMON_DEBUG_SHOW_LIGHTING[5].x == 0.0f;   
//}
//// ----------------------------------------------------------------------------
//bool IsVisualizeSpecDirMode (void)
//{
//   return PS_REG_COMMON_DEBUG_SHOW_LIGHTING[2].z == 1.0f;   
//}
//// ----------------------------------------------------------------------------
//bool IsVisualizeSpecColMode (void)
//{
//   return PS_REG_COMMON_DEBUG_SHOW_LIGHTING[2].w == 1.0f;   
//}
//// ----------------------------------------------------------------------------
//bool IsUseCheckerMode (void)
//{
//   return PS_REG_COMMON_DEBUG_SHOW_LIGHTING[1].w == 1.0f;   
//}
//// ----------------------------------------------------------------------------
//bool IsUseLightingMode (void)
//{
//   return PS_REG_COMMON_DEBUG_SHOW_LIGHTING[0].x > 0.0f;
//}
//// ----------------------------------------------------------------------------
//bool IsCubeMapReflCoefAdvanced (void)
//{
//   return PS_REG_COMMON_DEBUG_SHOW_LIGHTING[5].y > 0.0f;
//}
//// ----------------------------------------------------------------------------
//half4 GetDebugColorLM (void)
//{
//   half4 res = LIGHT_COLOR_WHITE * PS_REG_COMMON_DEBUG_SHOW_LIGHTING[0].y;
//   return res;
//}

// akParams - x : ak_ref, y : min ak_ref, y : scale, z : bias
float AKillRefByDist(in float4 akParams, in float camZ) {
   float t = saturate(akParams.z * camZ + akParams.w);
   return lerp(abs(akParams.y), akParams.x, t);
}

float3 DebugColorizeValue(float v, float vmin, float vmax)
{
	v = max(v, 0.0);
	v = (v - vmin) / (vmax - vmin); // value to [0 1];
	float3 f = saturate(v * 3.0 - float3(0, 1.0, 2.0));
	float3 color;
	color = lerp(LIGHT_COLOR_GREEN.rgb, LIGHT_COLOR_BLUE.rgb,    f.x);
	color = lerp(color,                 LIGHT_COLOR_YELLOW.rgb,  f.y);
	color = lerp(color,                 LIGHT_COLOR_RED.rgb,     f.z);
	return color;
}



//float3 ApplyViewMat (in float4 src)
//{
//   return float3(
//      dot(src, COMMON_VIEW_MATRIX[0]),
//      dot(src, COMMON_VIEW_MATRIX[1]),
//      dot(src, COMMON_VIEW_MATRIX[2]));
//}
//
//
//float4 ApplyProjMat (in float4 src)
//{
//   float4 dest; 
//   dest.x = dot(src, COMMON_PROJ_MATRIX[0]);
//   dest.y = dot(src, COMMON_PROJ_MATRIX[1]);
//   dest.z = dot(src, COMMON_PROJ_MATRIX[2]);
//   dest.w = dot(src, COMMON_PROJ_MATRIX[3]); 
//   return dest;
//}
//
//TPL_COMMON_DATA getCommonTplData()
//{
//   TPL_COMMON_DATA tplData;
//   int idx = GET_INST_DATA(OBJ_DATA_BUFFER).PS_REG_COMMON_TPL_IDX.x;
//   tplData = TPL_COMMON_DATA_BUF[idx];
//   
//   return tplData;
//}


#if defined(_DEBUG)

void DbgVisGetOutput(inout float4 col, float4 diff, float3 norm, float localAO, float gloss, float3 metalness, float lmReflK, float nLayers, float windPower)
{
   uint dbgParam = asuint(PS_REG_COMMON_DEBUG_SHOW_LIGHTING[0].z);
   if (dbgParam != DBGVIS_NONE) {
      if (dbgParam == DBGVIS_ALBEDO) {
         col.rgb = diff.rgb;     // diffuse
      } else if (dbgParam == DBGVIS_GBUF_GLOSS) {
         col.rgb = SET_VECTOR(float3 ,gloss); // gbuf gloss
      } else if (dbgParam == DBGVIS_GBUF_MET) {
         col.rgb = metalness;   // gbuf metalness
      } else if (dbgParam == DBGVIS_GBUF_NORM_PIX) {
         col.rgb = norm * .5f + .5f; // gbuf normals
      } else if (dbgParam == DBGVIS_GBUF_LOCAL_AO) {
         col.rgb = FLOAT3(localAO); // local AO
      } else if (dbgParam == DBGVIS_LAYERS_COUNT) {
         col.rgb = DebugColorizeValue(nLayers, 1, 4); // glt layers count
      } else if (dbgParam == DBGVIS_DYN_LIGHTS_COUNT) {
         float maxCount = PS_REG_COMMON_DEBUG_SHOW_LIGHTING[1].z;
         maxCount = (maxCount == 0.0f) ? 8.0f : maxCount;
         col.rgb = ThermoGrad(gDebugDynLightsCount / maxCount);
      } else if (dbgParam == DBGVIS_REFLECTIONS_COUNT) {
         col.rgb = DebugColorizeValue(gDebugReflectionsCount, 0, 8); // reflection count
      } else if (dbgParam == DBGVIS_LIGHTMAP_REFL_K) {
         col.rgb = FLOAT3(lmReflK);
      } else if (dbgParam == DBGVIS_GLOBAL_WIND_POWER) {
         col.rgb = FLOAT3(windPower);
      }
   }
}

void DbgShowVertexFog(inout float4 col) {
   col.rgb = lerp(col.rgb, float3(5,0,0), PS_REG_COMMON_DEBUG_SHOW_LIGHTING[1].x * 0.5f);
}


void DbgObjectUnderCursor(in float2 maskTexCoord, in float depth, in uint lwiInstId)
{ 
   uint4 idData  = asuint(GET_INST_DATA(OBJ_DATA_BUFFER).PS_REG_COMMON_OBJ_CUBE_MASK);
   uint  lwiUID0, lwiUID1;
   if (lwiInstId != 0xFFFFFFFF) {
      lwiUID0 = LWI_INST_DEBUG_DATA_BUF[lwiInstId * CSII_STATIC_DEBUG_DATA_SIZE_U1];
      lwiUID1 = LWI_INST_DEBUG_DATA_BUF[lwiInstId * CSII_STATIC_DEBUG_DATA_SIZE_U1 + 1];
   } else {
      lwiUID0 = 0xFFFFFFFF;
      lwiUID1 = 0xFFFFFFFF;
   }
   
   uint  instID  = idData.y;
   uint  objID   = idData.z;
   uint  splitID = idData.w;
   
   float2 cursorPos = PS_REG_COMMON_DEBUG_SHOW_LIGHTING[2].xy;
   float2 tc        = abs(maskTexCoord.xy - cursorPos.xy);
   
#if !defined(_AP_NX64)
   HLSL_ATTRIB_BRANCH
   if (objID != 0xffffffff && cursorPos.x >= 0.0f && tc.x <= COMMON_VP_PARAMS[0].z && tc.y <= COMMON_VP_PARAMS[0].w) {
      uint idx = 0;
      RW_TEX_INTERLOCKED_ADD(UAV(texObjID), int2(0, 0), 1, idx); // storing buf counter in 0th element
      idx   = min(idx, OBJ_ID_BUF_SIZE);
      
      uint resID = OBJ_ID_DATA_SIZE_U1 + idx * OBJ_ID_DATA_SIZE_U1; // offset first element where counter stored.
      
      _RW_TEX_STORE(UAV(texObjID), int2(resID, 0),     instID);
      _RW_TEX_STORE(UAV(texObjID), int2(resID + 1, 0), (splitID << 16) + (objID & 0xFFFF));
      _RW_TEX_STORE(UAV(texObjID), int2(resID + 2, 0), lwiUID0);
      _RW_TEX_STORE(UAV(texObjID), int2(resID + 3, 0), lwiUID1);
      _RW_TEX_STORE(UAV(texObjID), int2(resID + 4, 0), asuint(depth));
   }
#endif
}

#endif


//int3 calcFroxelCoord(float2 maskTexCoord, float depth)
//{
//	int slice = int(max(log2(depth) * COMMON_LBUF_PARAMS[0].z + COMMON_LBUF_PARAMS[0].w, 0.0));
//   slice = min(slice, int(LBUF_NUM_Z_SLICES) - 1); // for last split
//   return int3(int2(maskTexCoord * COMMON_LBUF_PARAMS[0].xy) / DEPTH_Z_TILE_SIZE, slice);
//}
//
//float calcESM (in float occluderDepthExp, in float receiverDepth)
//{
//   float K = COMMON_REND_SCENE_PARAMS[1].x;
//   return saturate(exp(K * (occluderDepthExp - receiverDepth)));
//}
//
//float4 calcESM4 (in float4 occluderDepthExp, in float4 receiverDepth)
//{
//   float4 K = COMMON_REND_SCENE_PARAMS[1].xxxx;
//   return saturate(exp(K * (occluderDepthExp - receiverDepth)));
//}

#endif // _COMMON_PIXEL_SHADER_STUFF_INCLUDED_
