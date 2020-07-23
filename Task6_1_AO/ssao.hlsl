SamplerState gPointClampSampler : register(s0);

cbuffer cbPass : register(b0)
{
float4x4 projMatrix;
float4x4 projMatrixInv;
float width;
float height;
float radius;
float bias;
}

Texture2D depthStencil : register(t0);
Texture2D normalsRenderTarget : register(t1);
RWTexture2D<float1> ssaoOutput: register(u0);

static const float3 ssaoKernel[] = {
   0.049771, -0.044709, 0.049963,
   0.014575, 0.016531, 0.002239,
   -0.040648, -0.019375, 0.031934,
   0.013778, -0.091582, 0.040924,
   0.055989, 0.059792, 0.057659,
   0.092266, 0.044279, 0.015451,
   -0.002039, -0.054402, 0.066735,
   -0.000331, -0.000187, 0.000369,
   0.050045, -0.046650, 0.025385,
   0.038128, 0.031402, 0.032868,
   -0.031883, 0.020459, 0.022515,
   0.055702, -0.036974, 0.054492,
   0.057372, -0.022540, 0.075542,
   -0.016090, -0.003768, 0.055473,
   -0.025033, -0.024829, 0.024951,
   -0.033688, 0.021391, 0.025402,
   -0.017530, 0.014386, 0.005348,
   0.073359, 0.112052, 0.011015,
   -0.044056, -0.090284, 0.083683,
   -0.083277, -0.001683, 0.084987,
   -0.010406, -0.032867, 0.019273,
   0.003211, -0.004882, 0.004164,
   -0.007383, -0.065835, 0.067398,
   0.094141, -0.007998, 0.143350,
   0.076833, 0.126968, 0.106999,
   0.000393, 0.000450, 0.000302,
   -0.104793, 0.065445, 0.101737,
   -0.004452, -0.119638, 0.161901,
   -0.074553, 0.034449, 0.224138,
   -0.002758, 0.003078, 0.002923,
   -0.108512, 0.142337, 0.166435,
   0.046882, 0.103636, 0.059576,
   0.134569, -0.022512, 0.130514,
   -0.164490, -0.155644, 0.124540,
   -0.187666, -0.208834, 0.057770,
   -0.043722, 0.086926, 0.074797,
   -0.002564, -0.002001, 0.004070,
   -0.096696, -0.182259, 0.299487,
   -0.225767, 0.316061, 0.089156,
   -0.027505, 0.287187, 0.317177,
   0.207216, -0.270839, 0.110132,
   0.054902, 0.104345, 0.323106,
   -0.130860, 0.119294, 0.280219,
   0.154035, -0.065371, 0.229843,
   0.052938, -0.227866, 0.148478,
   -0.187305, -0.040225, 0.015926,
   0.141843, 0.047163, 0.134847,
   -0.044268, 0.055616, 0.055859,
   -0.023583, -0.080970, 0.219130,
   -0.142147, 0.198069, 0.005194,
   0.158646, 0.230457, 0.043715,
   0.030040, 0.381832, 0.163825,
   0.083006, -0.309661, 0.067413,
   0.226953, -0.235350, 0.193673,
   0.381287, 0.332041, 0.529492,
   -0.556272, 0.294715, 0.301101,
   0.424490, 0.005647, 0.117578,
   0.366500, 0.003588, 0.085702,
   0.329018, 0.030898, 0.178504,
   -0.082938, 0.512848, 0.056555,
   0.867363, -0.002734, 0.100138,
   0.455745, -0.772006, 0.003841,
   0.417290, -0.154846, 0.462514,
   -0.442722, -0.679282, 0.186503,
};

static const float3 ssaoNoise[] = {
   -0.729046, 0.629447, 0.000000,
   0.670017, 0.811584, 0.000000,
   0.937736, -0.746026, 0.000000,
   -0.557932, 0.826752, 0.000000,
   -0.383666, 0.264719, 0.000000,
   0.094441, -0.804919, 0.000000,
   -0.623236, -0.443004, 0.000000,
   0.985763, 0.093763, 0.000000,
   0.992923, 0.915014, 0.000000,
   0.935390, 0.929777, 0.000000,
   0.451678, -0.684774, 0.000000,
   0.962219, 0.941186, 0.000000,
   -0.780276, 0.914334, 0.000000,
   0.596212, -0.029249, 0.000000,
   -0.405941, 0.600561, 0.000000,
   -0.990433, -0.716227, 0.000000,
};

float3 getPosFromNdc(uint2 dTid)
{
   float depthSample = depthStencil.GatherRed(gPointClampSampler, dTid / float2(width, height), int2(0, 0));
   float4 ndcPos = float4(dTid / float2(width, height) * 2.f - 1.f, depthSample, 1.f);
   float4 viewPos = mul(projMatrixInv, ndcPos);
   viewPos.y = -viewPos.y;
   if (depthSample == 1.f)
      viewPos = float4(1, 1, 1, 1);
   return viewPos.xyz / viewPos.w;
}

[numthreads(8, 8, 1)]
void main(uint3 dTid : SV_DispatchThreadID, uint2 gTid : SV_GroupThreadID)
{
   float depthSample = depthStencil.GatherRed(gPointClampSampler, dTid.xy / float2(width, height), int2(0, 0));
   if (depthSample == 1.f)
   {
      ssaoOutput[dTid.xy] = 1.f;
      return;
   }
   float3 pos = getPosFromNdc(dTid.xy);
   float3 normal = normalsRenderTarget.SampleLevel(gPointClampSampler, dTid.xy / float2(width, height), 0).xyz;
   float3 randomVec = normalize(ssaoNoise[(gTid.x * 4 + gTid.y * 4) % 16]);

   float3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
   float3 biTangent = cross(normal, tangent);
   float3x3 tbn = transpose(float3x3(tangent, biTangent, normal));
   uint kernel = 64;

   float occlusion = 0.f;
   float rangeCheck;
   float depth;
   float3 sampleTap;
   float4 offset;
   for (int i = 0; i < kernel; ++i)
   {
      sampleTap = mul(tbn, ssaoKernel[i % 64]);
      sampleTap = pos + sampleTap * radius;

      offset = mul(projMatrix, float4(sampleTap, 1.f));
      offset.xyz /= offset.w;
      offset.xy = offset.xy * 0.5 + 0.5;
      offset.y = 1.f - offset.y;
      depth = getPosFromNdc(offset.xy * float2(width, height)).z;
      rangeCheck = clamp(smoothstep(0.0, 1.0, radius / abs(pos.z - depth)), 0.f, 1.f) * (abs(offset.x - 0.5f) + 0.5f) * (abs(offset.y - 0.5f) + 0.5f);
      occlusion += (depth >= sampleTap.z - bias ? 0.f : 1.f) * rangeCheck;
   }
   occlusion = 1.f - occlusion / float(kernel);

   ssaoOutput[dTid.xy] = occlusion;
}
