#ifdef _API_DX10
#error @INVALID_DEFINES@ // depth gather4
#endif

#define SAMP_SSAO_CLAMP PS_SAMPLERS[PS_SMP_CLAMP_LINEAR]

#if defined(SSAO_BLUR)

float4 ssao_apply_gamma(in float4 mask)
{
   return pow(mask, FLOAT4(PS_REG_SSAO_COMMON_PARAMS[0].x));
}

float3 RayViewToProjection(float3 ray, float3 pos, float2 oneDivHalfFovTan)
{
   float3 result;
   result.xy = ray.xy * pos.z - pos.xy * ray.z;
   result.xy *= oneDivHalfFovTan;
   result.xy /= pos.z * (pos.z + ray.z);
   result.z = - ray.z / pos.z / (pos.z + ray.z);
   return result;
}


half4 SSAOFunc(float2 pixelCoord, float2 maskUV)
{   
   float2 texUV = maskUV;

   // YX order
   float4 v[3][3];
   float  d[3][3];
   
   float4  dd;

   //vv = SAMPLE_GATHER(SSAO_MASK,  texUV - PS_REG_SSAO_SCREEN[0].zw);
   for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
         v[i][j] = SAMPLE_LEVEL(SSAO_MASK,  texUV + PS_REG_SSAO_SCREEN[0].zw * float2(i - 1, j - 1), 0);
         v[i][j].xyz = normalize(v[i][j].xyz * 2.f - 1.f);
      }
   }
   
   dd = SAMPLE_GATHER(SSAO_DEPTH, texUV - PS_REG_SSAO_SCREEN[0].zw);
   d[1][0] = dd.r, d[1][1] = dd.g, d[0][1] = dd.b, d[0][0] = dd.a;   
   
   dd = SAMPLE_GATHER(SSAO_DEPTH, texUV + PS_REG_SSAO_SCREEN[0].zw * float2(1, -1));
   d[1][2] = dd.g, d[0][2] = dd.b;
   
   dd = SAMPLE_GATHER(SSAO_DEPTH, texUV + PS_REG_SSAO_SCREEN[0].zw * float2(-1, 1));
   d[2][0] = dd.r, d[2][1] = dd.g;
   
   d[2][2] = SAMPLE_LEVEL(SSAO_DEPTH, texUV + PS_REG_SSAO_SCREEN[0].xy, 0).r;
   
   const int4 offsX = int4(-1, -1,  0,  1);
   const int4 offsY = int4( 0, -1, -1, -1); 
   
   float4 tv = v[1][1];
   float tn = 1;

   HLSL_ATTRIB_UNROLL
   for (int i = 0 ; i < 4 ; i++) {
      float dd1 = abs(d[1 + offsX[i]][1 + offsY[i]] - d[1][1]);
      float dd2 = abs(d[1 - offsX[i]][1 - offsY[i]] - d[1][1]);
      
      float4 vv1 = v[1 + offsX[i]][1 + offsY[i]];
      float4 vv2 = v[1 - offsX[i]][1 - offsY[i]];

      if (abs(dd1 - dd2) < 1e-4) {
         tv += vv1 + vv2, tn += 2;
      } else {
         tv += dd1 < dd2 ? vv1 : vv2, tn++;
      }
   }
   
   tv /= tn;
   
   #ifdef SSAO_APPLY_GAMMA   
      tv.w = ssao_apply_gamma(saturate(tv).wwww).w;
   #endif
   tv.xyz = normalize(tv.xyz) * 0.5f + 0.5f;

   return tv;
}

#elif defined(SSAO_UPSCALE)

half4 SSAOFunc(float2 pixelCoord, float2 maskUV)
{
   float2 uv = maskUV;
   // xy = 0.5 pixel offset, zw = 1.5 pixel offset
   float4 offset = SSAO_TEX_COORD_SCALE[0];
   float2 twoPixels = SSAO_TEX_COORD_SCALE[0].xy + SSAO_TEX_COORD_SCALE[0].zw;
   float2 onePixel = SSAO_TEX_COORD_SCALE[0].zw - SSAO_TEX_COORD_SCALE[0].xy;
   float2 uvFloor = floor((uv - onePixel) / twoPixels) * twoPixels + onePixel;
   float2 uvCeil  = uvFloor + twoPixels;
   //uvFloor = max(uvFloor, onePixel);
   //uvCeil  = min(uvCeil, 1 - onePixel);
   // Four half-res neighbours of our full-res pixel
   // 0 | 1
   // 2 | 3
   float2 uv0 = uvFloor;
   float2 uv1 = float2(uvCeil.x, uvFloor.y);
   float2 uv2 = float2(uvFloor.x, uvCeil.y);
   float2 uv3 = uvCeil;

   float depthFS          = sample_z_lod(_TEX_SAMP(SSAO_DEPTH), uv);
   //float4 depthsHS = PS_TEXTURES_2D[PS_SSAO_DEPTH_HALF_TEX].Gather(PS_SAMPLERS[PS_SSAO_DEPTH_HALF_SMP], input.texCoord3.zw);
   float depthTopLeft     = sample_z_lod(_TEX_SAMP(SSAO_DEPTH_HALF), uv0);
   float depthTopRight    = sample_z_lod(_TEX_SAMP(SSAO_DEPTH_HALF), uv1);
   float depthBottomLeft  = sample_z_lod(_TEX_SAMP(SSAO_DEPTH_HALF), uv2);
   float depthBottomRight = sample_z_lod(_TEX_SAMP(SSAO_DEPTH_HALF), uv3);
   float k_hor = (uv.x - uv0.x) / (uv3.x - uv0.x);
   float k_ver = (uv.y - uv0.y) / (uv3.y - uv0.y);
   
   float depthLeft  = depthTopLeft  * (1 - k_ver) + depthBottomLeft  * k_ver;
   float depthRight = depthTopRight * (1 - k_ver) + depthBottomRight * k_ver;
   float depthCenter = depthLeft * (1 - k_hor) + depthRight * k_hor;
   //return (uv.y - uv0.y) * 1000;
   float depthDiff = abs(depthFS - depthCenter) * z_to_w(depthFS);
   #ifdef VID_USE_INVERTED_Z
   if (depthFS < PS_REG_SSAO_PARAMS[0].w) {
   #else
   if (depthFS > PS_REG_SSAO_PARAMS[0].w) {
   #endif
      depthDiff *= 2048;// empirical eye-tuned value for the scene
   } else {
      depthDiff *= 131072;// empirical eye-tuned value for first person weapon with compressed z
   }
   float2 sampleUV = uv;
   if (depthDiff > 1) {
      float2 uvMin;
      float depthMin, d;
      uvMin = uv0;
      depthMin = abs(depthFS - depthTopLeft);
      {
         d = abs(depthFS - depthTopRight);
         if (d < depthMin) {
            uvMin = uv1;
            depthMin = d;
         }
      }
      {
         d = abs(depthFS - depthBottomLeft);
         if (d < depthMin) {
            uvMin = uv2;
            depthMin = d;
         }
      }
      {
         d = abs(depthFS - depthBottomRight);
         if (d < depthMin) {
            uvMin = uv3;
            depthMin = d;
         }
      }
      sampleUV = uvMin;
   }
   
   half4 ssaoMask = _SAMPLE_LEVEL(_TEX_ID(SSAO_MASK), SAMP_SSAO_CLAMP, sampleUV, 0);

   // float amount = PS_REG_SSAO_COMMON_PARAMS[0].x;
   // tv = pow(saturate(tv), amount);

   return ssaoMask;
}

#else

#error @INVALID_DEFINES@

#endif

