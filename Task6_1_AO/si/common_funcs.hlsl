#ifndef _COMMON_FUNCS_VSH_INCLUDED_
#define _COMMON_FUNCS_VSH_INCLUDED_
/***************************************************************
*                       COMMON_FUNCS.FX
* DESCRIPTION:
*  This file contains common shader functions that don't depent 
*  on defines and shader type (!)
*
***************************************************************/
#include "registers_common.hlsl"
//#include <sh.fx>


#define PI 3.14159265f
#define RCP_2PI 0.15915494f // GCN constant literal

// ----------------------------------------------------------------------------
float pow2(float x) {
   return x * x;
}

float2 pow2(float2 x) {
   return x * x;
}

float3 pow2(float3 x) {
   return x * x;
}

float4 pow2(float4 x) {
   return x * x;
}


float pow4(in float x) {
   float x2 = x * x;
   return x2 * x2;
}

float pow8(in float x) {
   x *= x;// x^2
   x *= x;// x^4
   x *= x;// x^8
   return x;
}


/***************************************************************
*                    TANGENT SPACE FUNCTIONS
***************************************************************/
//static const float3 vect_unit_x = { 1, 0, 0 };
//static const float3 vect_unit_y = { 0, 1, 0 };
//static const float3 vect_unit_z = { 0, 0, 1 };
// ----------------------------------------------------------------------------
float3 normal_component (float3 vect, float3 norm)
{
   return norm * dot(vect, norm);
}

//// ----------------------------------------------------------------------------
//float3 tangent_component (float3 vect, float3 norm)
//{
//   return vect - normal_component(vect, norm);
//}

//// ----------------------------------------------------------------------------
//void calc_tangent_binormal (float3 norm, inout float3 tang, inout float3 binorm)
//{
//   float3   vx, vy, vect_unit;
//
//   if (abs(norm.y) < 0.00001 && abs(norm.z) < 0.00001) {
//      vect_unit = vect_unit_y;
//   } else {
//      vect_unit = vect_unit_x;
//   }
//   tang   = normalize(tangent_component(vect_unit, norm));
//   binorm = cross(tang, norm);
//}

//// ----------------------------------------------------------------------------
//float dist_point_plane(float3 pos, float4 plane)
//{
//   return dot(pos, plane.xyz) - plane.w;
//}

//// ----------------------------------------------------------------------------
//void build_matrix (float3 vx, float3 vy, float3 vz, inout float3 matr[3])
//{
//   matr[0] = vx;
//   matr[1] = vy;
//   matr[2] = vz;   
//}

//// ----------------------------------------------------------------------------
//void build_bump_matrix_N (float3 vz, inout float3 bump_matr[3])
//{
//   float3 vx = 0, vy = 0;
//   calc_tangent_binormal(vz, vy, vx);
//   build_matrix(vx, vy, vz, bump_matr);
//}

// ----------------------------------------------------------------------------
float3 calc_binormal (float3 norm, float4 tang)
{
   return cross(norm, tang.xyz) * tang.w;
}

//// ----------------------------------------------------------------------------
//void _build_bump_matrix_NT (float3 norm, float4 tang, inout float3 bump_matr[3])
//{
//   float3 binorm;
//   binorm = calc_binormal(norm, tang);
//   build_matrix(tang.xyz, binorm, norm, bump_matr);
//}

// -----------------------------------------------------------------------------
float3 world_to_tang(float3 v, float3 T, float3 B, float3 N)
{
   return float3(dot(v,T), dot(v,B), dot(v,N));
}

// ----------------------------------------------------------------------------
float3 world_to_tang(in float3 v, in float3 TBN[3])
{
   return world_to_tang(v, TBN[0], TBN[1], TBN[2]);
}

// ----------------------------------------------------------------------------
float3 tang_to_world(in float3 v, in float3 TBN[3])
{
   return v.x * TBN[0] + v.y * TBN[1] + v.z * TBN[2];
}



/***************************************************************
*                    OTHER FUNCTIONS
***************************************************************/
// ----------------------------------------------------------------------------
//float4 apply_view_proj_matrix_world_no_jitter(float4 wpos)
//{
//   return ISOLATE_VALUE(float4(dot(wpos, COMMON_VIEWPROJ_MATRIX_NO_JIT[0]),
//                               dot(wpos, COMMON_VIEWPROJ_MATRIX_NO_JIT[1]),
//                               dot(wpos, COMMON_VIEWPROJ_MATRIX_NO_JIT[2]),
//                               dot(wpos, COMMON_VIEWPROJ_MATRIX_NO_JIT[3])));
//}

//float4 apply_view_proj_matrix_world_dir(float3 wvec)
//{  
//   #if defined(COMMON_VP_FPS_MODEL)
//      float4 result = ISOLATE_VALUE(float4(dot(wvec, COMMON_FPMODEL_VIEWPROJ_MATRIX[0].xyz),
//                                           dot(wvec, COMMON_FPMODEL_VIEWPROJ_MATRIX[1].xyz),
//                                           dot(wvec, COMMON_FPMODEL_VIEWPROJ_MATRIX[2].xyz),
//                                           dot(wvec, COMMON_FPMODEL_VIEWPROJ_MATRIX[3].xyz)));
//   #else
//      float4 result = ISOLATE_VALUE(float4(dot(wvec, COMMON_VIEWPROJ_MATRIX[0].xyz),
//                                           dot(wvec, COMMON_VIEWPROJ_MATRIX[1].xyz),
//                                           dot(wvec, COMMON_VIEWPROJ_MATRIX[2].xyz),
//                                           dot(wvec, COMMON_VIEWPROJ_MATRIX[3].xyz)));
//   #endif
//   return result;
//}

//float4 apply_view_proj_matrix_world(float4 wpos)
//{
//   // DS: This is special workaround for precision issue with 
//   //     big numbers in view-projection matrix due to big (1000.f+) 
//   //     camera offset from world zero. We can gain back precision
//   //     if we remove translation component from view matrix and 
//   //     therefore replace dot4(big_number) with add(big_number) 
//   //     followed by dot4(small_number)
//   wpos.xyz -= COMMON_VIEW_POSITION[0].xyz;
//   
//   #if defined(COMMON_VP_FPS_MODEL)
//      float4 result = ISOLATE_VALUE(float4(dot(wpos, COMMON_FPMODEL_VIEWPROJ_MATRIX[0]),
//                                           dot(wpos, COMMON_FPMODEL_VIEWPROJ_MATRIX[1]),
//                                           dot(wpos, COMMON_FPMODEL_VIEWPROJ_MATRIX[2]),
//                                           dot(wpos, COMMON_FPMODEL_VIEWPROJ_MATRIX[3])));
//   #else
//      float4 result = ISOLATE_VALUE(float4(dot(wpos, COMMON_VIEWPROJ_MATRIX[0]),
//                                           dot(wpos, COMMON_VIEWPROJ_MATRIX[1]),
//                                           dot(wpos, COMMON_VIEWPROJ_MATRIX[2]),
//                                           dot(wpos, COMMON_VIEWPROJ_MATRIX[3])));
//   #endif
//   return result;
//}

#define INVALID_VELOCITY float2(2.f, 2.f)


//float3 calc_vel_proj(in float4 wposPrev)
//{
//   #if defined(COMMON_VP_FPS_MODEL)
//      wposPrev.xyz -= COMMON_VIEW_POSITION[0].xyz;
//      wposPrev = float4(
//         dot(wposPrev, COMMON_FPMODEL_VIEWPROJ_MATRIX[0]),
//         dot(wposPrev, COMMON_FPMODEL_VIEWPROJ_MATRIX[1]),
//         dot(wposPrev, COMMON_FPMODEL_VIEWPROJ_MATRIX[2]),
//         dot(wposPrev, COMMON_FPMODEL_VIEWPROJ_MATRIX[3]));
//   #else
//      wposPrev = float4(
//         dot(wposPrev, COMMON_VIEWPROJ_MATRIX_PREV[0]),
//         dot(wposPrev, COMMON_VIEWPROJ_MATRIX_PREV[1]),
//         dot(wposPrev, COMMON_VIEWPROJ_MATRIX_PREV[2]),
//         dot(wposPrev, COMMON_VIEWPROJ_MATRIX_PREV[3]));
//   #endif
//   // TODO: Fix for LWI
//   wposPrev.w = wposPrev.w;//lerp(wposPrev.w, -1.0f, INST_REGS(CB_PIX_OBJ, PS_REG_COMMON_OBJ_SKIN_OFFSET, 0).w);
//   
//   return wposPrev.xyw;
//}

float2 velocity_compress(in float2 vel)
{
   vel.x = vel.x >= 0 ? sqrt(abs(vel.x)) : -sqrt(abs(vel.x));
   vel.y = vel.y >= 0 ? sqrt(abs(vel.y)) : -sqrt(abs(vel.y));;
   vel = vel * 127.f/255.f + 127.f/255.f;
   return vel;
}

float2 velocity_decompress(float2 vel)
{
   vel = vel * 255.f/127.f - 1.f;
   vel.x = vel.x >= 0 ? pow2(vel.x) : -pow2(vel.x);
   vel.y = vel.y >= 0 ? pow2(vel.y) : -pow2(vel.y);
   return vel;
}

//float2 calc_velocity(in float2 coordCur, in float3 projPrev)
//{
//   float2 screenPosCur  = coordCur.xy;
//   
//   float2 screenPosPrev = (float2(0.5f, -0.5f) * (projPrev.xy / projPrev.z) + float2(0.5f, 0.5f)) / COMMON_VP_PARAMS[0].xy;
//   float2 res = clamp((screenPosCur - screenPosPrev) / MAX_VELOCITY_PIXELS, FLOAT2(-1.0f), FLOAT2(1.0f));
//   return projPrev.z <= 0.0f ? INVALID_VELOCITY : velocity_compress(res);
//}

// ----------------------------------------------------------------------------
float4 tranform_vector4 (float4 vec, float4 matr_0, float4 matr_1, float4 matr_2)
{
   float4 v4;
  
   v4.x = dot(vec, matr_0);
   v4.y = dot(vec, matr_1);
   v4.z = dot(vec, matr_2);
   v4.w = 1;
   return v4;
}

// ----------------------------------------------------------------------------
float3 tranform_vector3 (float3 vec, float3 matr_0, float3 matr_1, float3 matr_2)
{
   float3 v3;
   v3.x = dot(vec, matr_0);
   v3.y = dot(vec, matr_1);
   v3.z = dot(vec, matr_2);
        
   return v3;
}


// ----------------------------------------------------------------------------
// see: https://seblagarde.wordpress.com/2014/12/01/inverse-trigonometric-functions-gpu-optimization-for-amd-gcn-architecture/
float fast_acos(float x)
{
   const float C0 = 1.56467;
   const float C1 = -0.155972;
   float y = abs(x);
   float res = C1 * y + C0;
   res *= sqrt(1.0f - y);

   return (x >= 0) ? res : PI - res;
}


// ----------------------------------------------------------------------------
// Common lihgting functions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
float shGetCoef(half3 nrm, float4 coefs)
{
   float res    = dot(float4(nrm, 1), coefs);
   return max(res, coefs.a * 1);
}
/***************************************************************
*                    QUATERNIONS
***************************************************************/
float3x4 quatAsMatrix(float4 q)
{
   float3x4 res = ZERO_OUT(float3x4);

   const float s = 2;/* / dot(q, q)*/
   
   float xs = q.x * s;
   float ys = q.y * s;
   float zs = q.z * s;
   
   float wx = q.w * xs;
   float wy = q.w * ys;
   float wz = q.w * zs;
   
   float xx = q.x * xs;
   float xy = q.x * ys;
   float xz = q.x * zs;
   
   float yy = q.y * ys;
   float yz = q.y * zs;
   float zz = q.z * zs;
   
   res[0][0] = 1 - yy - zz;
   res[1][0] = xy + wz;
   res[2][0] = xz - wy;
   
   res[0][1] = xy - wz;
   res[1][1] = 1 - xx - zz;
   res[2][1] = yz + wx;
   
   res[0][2] = xz + wy;
   res[1][2] = yz - wx;
   res[2][2] = 1 - xx - yy;
   
   return res;
}

bool dualQuatIsUnit(float4 q0, float4 qe)
{
   // assume q0 is normalized
   return abs(dot(q0,qe)) < 1e-5;
}

float4 quatConjugate(float4 q)
{
   return float4(-q.xyz, q.w);
}

float4 quatMul(float4 a, float4 b)
{
   float4 r;
   
   r.w = a.w * b.w - dot(a.xyz, b.xyz);
   r.xyz = cross(a.xyz, b.xyz);
   r.xyz += a.xyz * b.w;
   r.xyz += b.xyz * a.w;
   
   return r;
}

float3 dualQuatGetTranslationFromUnit(float4 q0, float4 qe)
{
   return 2 * quatMul(qe, quatConjugate(q0)).xyz;
}

float3 dualQuatGetTranslation(float4 q0, float4 qe)
{
   if (dualQuatIsUnit(q0, qe)) {
      return dualQuatGetTranslationFromUnit(q0, qe);
   }
   
   float scale = -2.0 / dot(q0, q0);

   return scale * float3(
      qe.w * q0.x - q0.w * qe.x + qe.y * q0.z - q0.y * qe.z,
      qe.w * q0.y - qe.x * q0.z + q0.x * qe.z - q0.w * qe.y,
      qe.w * q0.z - q0.x * qe.y - q0.w * qe.z + qe.x * q0.y);
}

float3x4 dualQuatAsMatrix(float4 q0, float4 qe)
{
   //@   // Fill in rotation part
   //@   MMatrix mat       = getRotation().asMatrix();
   float3x4 mat = quatAsMatrix(q0);

   //@   MVector translation = getTranslation();
   float3 translation = dualQuatGetTranslation(q0, qe);
   mat[0].w = translation.x;
   mat[1].w = translation.y;
   mat[2].w = translation.z;
   
   return mat;
}

void dualQuatNormalize(inout float4 q0, inout float4 qe)
{
   float invLen = 1 / length(q0);
   
   q0    *= invLen;
   qe    *= invLen;

   qe = qe - dot(q0, qe) * q0;
}

float2x4 dualQuatMul(float4 q0, float4 qe, float4 w0, float4 we)
{
   float2x4 res;

   res[0] = quatMul(q0, w0);

   res[1] = quatMul(qe, w0);
   res[1] += quatMul(q0, we);
   
   return res;
}

float3 dualQuatTransform(float4 q0, float4 qe, float3 v)
{
	//@rcMQuaternion q = mReal;
	float4 q = q0;
	//@MVector p(inPoint);
	float3 p = v;
	
	//@MVector qxyz(mReal.x,mReal.y,mReal.z);
	float3 qxyz = q0.xyz;

	//@MVector qxyzcrossp_plus_qwdotp = (qxyz^p) + q.w*p;
	float3 qxyzcrossp_plus_qwdotp = cross(qxyz, p) + q.w * p;
	
	//@p+=2.0*(qxyz^qxyzcrossp_plus_qwdotp);
	p += 2 * cross(qxyz, qxyzcrossp_plus_qwdotp);

	//@MVector dxyz(mDual.x,mDual.y,mDual.z);
	float3 dxyz = qe.xyz;
	
	//@MVector t = dxyz^qxyz;
	float3 t = cross(dxyz, qxyz);
	
	//@t+= mDual.w*qxyz;
	t += qe.w * qxyz;
	
	//@t+= -mReal.w*dxyz;
	t += -q0.w * dxyz;

	//@p+=-2.0*t;
	p += -2.0 * t;
	
	return p;

/*
   float4 qd_q0 = float4(0, 0, 0, 1);
   float4 qd_qe = float4(v, 0);

   float2x4 qd1 = dualQuatMul(q0, qe, qd_q0, qd_qe);

   float4 w0 = quatConjugate(q0);
   float4 we = - quatConjugate(qe);

   qd1 = dualQuatMul(qd1[0], qd1[1], w0, we);

   return qd1[1].xyz;
*/   
}

float4 quatNormalize(float4 quat)
{
   return quat / length(quat);
}

float4 quatMultiply(float4 a, float4 b)
{
   float as = a.w;
   float bs = b.w;
   
   float3 av = a.xyz;
   float3 bv = b.xyz;
   
   float4 res = ZERO_OUT(float4);
   res.w = as * bs - dot(av, bv);

   float3 tmp = cross(av, bv);

   float3 v = av * bs;
   res.xyz = v + tmp;

   v = bv * as;
   res.xyz += v;
   
   return res;
}

// ----------------------------------------------------------------------------
float3 quatTransform(float4 quat, float3 pt)
{
   float4 qV = ZERO_OUT(float4); qV.xyz = pt;

   float4 quatInv = quatConjugate(quat);

   float4 quatTmp = quatMultiply(quat, qV);
   qV = quatMultiply(quatTmp, quatInv);

   return qV.xyz;
}

// ----------------------------------------------------------------------------
void QuatFromAxisAngle(float3 axis, float angle, out float4 quat)
{
   float sinAngle;
   sincos(angle * 0.5f, sinAngle, quat.w);
   quat.xyz = sinAngle * axis;
}

// ----------------------------------------------------------------------------
half3 RGBDiv2HDR(in float4 rgbdiv)
{
   float m = rgbdiv.a * rgbdiv.a;

   m *= m;
   return rgbdiv.xyz * 0.5f / m;
}

// ----------------------------------------------------------------------------
float4 HDR2RGBDiv(in float3 hdr)
{
   float m = max(hdr.r, max(hdr.g, hdr.b));
   float4 res;

   res.rgb = hdr / m;
   HLSL_ATTRIB_FLATTEN
   if (m > 0.5f) {
      res.a = sqrt(sqrt(0.5f / m));
   }
   return res;
}

// ----------------------------------------------------------------------------
float luminance (float3 c)
{
   return dot(c, float3(0.212671, 0.715160, 0.072169));
}

// ----------------------------------------------------------------------------
float SmoothStep(float x)
{
   return x * x * (3 - 2 * x);
}

// ----------------------------------------------------------------------------
float SmootherStep(float x)
{
   return x * x * x * (10 + x * (6 * x - 15));
}


//void UnpackLwiMatrix (in uint instID, out float3x4 wMatr)
//{
//   uint blockIdx     = instID / 4; 
//   uint instIdx      = instID % 4;
//   uint dataOffset   = blockIdx * 5;
//   
//   int  instOffset   = dataOffset + 1 + instIdx;
//   float4 matr_quat  = LWI_INST_DATA_BUF[instOffset];
//   float3 pos        = matr_quat.xyz;
//   
//   uint   qPack      = asuint(matr_quat.w);
//   float4 q          = (float4(qPack & 0xFF, (qPack >> 8) & 0xFF, (qPack >> 16) & 0xFF, (qPack >> 24) & 0xFF) * (2.0f / 255.f)) - 1.0f;
//
//   uint   scalePack  = asuint(LWI_INST_DATA_BUF[dataOffset][instIdx]);
//   float  maxScale   = float((scalePack >> 24) & 0xFF);
//   float3 scale      = float3(scalePack & 0xFF, (scalePack >> 8) & 0xFF, (scalePack >> 16) & 0xFF) * (maxScale / 255.f);
//   
//   const float s = 2;
//   float xs = q.x * s;
//   float ys = q.y * s;
//   float zs = q.z * s;
//              
//   float wx = q.w * xs;
//   float wy = q.w * ys;
//   float wz = q.w * zs;
//              
//   float xx = q.x * xs;
//   float xy = q.x * ys;
//   float xz = q.x * zs;
//              
//   float yy = q.y * ys;
//   float yz = q.y * zs;
//   float zz = q.z * zs;
//
//   wMatr[0][0] = (1 - yy - zz) * scale.x;
//   wMatr[1][0] = (xy + wz    ) * scale.x;
//   wMatr[2][0] = (xz - wy    ) * scale.x;
//      
//   wMatr[0][1] = (xy - wz    ) * scale.y;
//   wMatr[1][1] = (1 - xx - zz) * scale.y;
//   wMatr[2][1] = (yz + wx    ) * scale.y;
//   
//   wMatr[0][2] = (xz + wy    ) * scale.z;
//   wMatr[1][2] = (yz - wx    ) * scale.z;
//   wMatr[2][2] = (1 - xx - yy) * scale.z;
//   
//   wMatr[0][3] = pos.x;
//   wMatr[1][3] = pos.y;
//   wMatr[2][3] = pos.z;
//}


#endif// _COMMON_FUNCS_VSH_INCLUDED_
