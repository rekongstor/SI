#ifndef _COMMON_MATH_FX_
#define _COMMON_MATH_FX_

#define M3D_PI       3.14159265f
#define M3D_2_PI     6.28318531f
#define M3D_HALF_PI  (M3D_PI * 0.5f)
#define M3D_RCP_2_PI 0.15915494f // GCN constant literal
#define M3D_EPSILON  0.000001f

// this creates the standard Hessian-normal-form plane equation from three points, 
// except it is simplified for the case where the first point is the origin
float3 CreatePlaneEquation (float3 b, float3 c)
{
	return normalize(cross(b, c));
}


// point-plane distance, simplified for the case where 
// the plane passes through the origin
float GetSignedDistanceFromPlane (float3 p, float3 eqn)
{
    // dot( eqn.xyz, p.xyz ) + eqn.w, , except we know eqn.w is zero 
    // (see CreatePlaneEquation above)
    return dot(eqn, p);
}


bool TestSpheresIntersection (in float3 org0, in float r0, in float3 org1, in float r1)
{
   float rSum = r0 + r1;
   float3 vOrg = org0 - org1;
   return dot(vOrg, vOrg) < rSum * rSum;
}


bool TestSphereConeIntersection (in float3 sOrg, in float sRadius, in float3 coneOrg,
                                 in float3 coneDir, in float coneAngleSin, in float coneAngleCos)
{
   float3 u  = sOrg - coneOrg;
   float3 d  = u + (sRadius / coneAngleSin) * coneDir;
   float dSq = dot(d, d);
   float e   = dot(coneDir, d);
   
   if (e > 0 && e * e >= dSq * coneAngleCos * coneAngleCos) {
      dSq =  dot(u, u);
      e   = -dot(coneDir, u);
      
      if (e > 0 && e * e >= dSq * coneAngleSin * coneAngleSin) {
         return dSq <= sRadius * sRadius;
      } else {
         return true;
      }
   }
   return false;
}


bool TestFrustumSphereIntersection (in float3 frustum[4], in float3 org, in float radius) {
  return (GetSignedDistanceFromPlane(org, frustum[0]) < radius) &&
         (GetSignedDistanceFromPlane(org, frustum[1]) < radius) &&
         (GetSignedDistanceFromPlane(org, frustum[2]) < radius) &&
         (GetSignedDistanceFromPlane(org, frustum[3]) < radius);
}

#endif