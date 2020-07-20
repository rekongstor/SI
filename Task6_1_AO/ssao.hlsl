SamplerState gPointClampSampler : register(s0);
SamplerState gPointMirrorSampler : register(s1);
SamplerState gLinearClampSampler : register(s2);
SamplerState gViewSpaceDepthTapSampler : register(s3);
SamplerState gZeroTextureSampler : register(s4);

cbuffer cbPass : register(b0)
{
float4x4 projMatrix;
float width;
float height;
}

Texture2D<float1> depthStencil : register(t0);
Texture2D<float4> normalsRenderTarget : register(t1);
Texture2D<float4> positionRenderTarget : register(t2);
RWTexture2D<float1> ssaoOutput: register(u0);

static const float3 ssaoKernel[] =
{
   0.497709, -0.447092, 0.499634,
   0.145427, 0.164949, 0.022337,
   -0.402936, -0.192060, 0.316554,
   0.135109, -0.898060, 0.401306,
   0.540875, 0.577609, 0.557006,
   0.874615, 0.419730, 0.146465,
   -0.018898, -0.504141, 0.618431,
   -0.002984, -0.001691, 0.003334,
   0.438746, -0.408985, 0.222553,
   0.323672, 0.266571, 0.279020,
   -0.261392, 0.167732, 0.184589,
   0.440034, -0.292085, 0.430474,
   0.435821, -0.171226, 0.573847,
   -0.117331, -0.027480, 0.404520,
   -0.174974, -0.173549, 0.174403,
   -0.225430, 0.143145, 0.169986,
   -0.112191, 0.092068, 0.034229,
   0.448674, 0.685331, 0.067367,
   -0.257349, -0.527384, 0.488827,
   -0.464402, -0.009388, 0.473936,
   -0.055382, -0.174926, 0.102575,
   0.016309, -0.024795, 0.021147,
   -0.035780, -0.319047, 0.326624,
   0.435365, -0.036990, 0.662937,
   0.339125, 0.560410, 0.472273,
   0.001655, 0.001895, 0.001271,
   -0.421643, 0.263322, 0.409346,
   -0.017109, -0.459828, 0.622265,
   -0.273823, 0.126528, 0.823235,
   -0.009685, 0.010807, 0.010262,
   -0.364436, 0.478037, 0.558969,
   0.150670, 0.333067, 0.191465,
   0.414059, -0.069268, 0.401582,
   -0.484817, -0.458746, 0.367069,
   -0.530125, -0.589921, 0.163190,
   -0.118435, 0.235465, 0.202611,
   -0.006663, -0.005200, 0.010577,
   -0.241253, -0.454733, 0.747212,
   -0.541038, 0.757421, 0.213657,
   -0.063346, 0.661410, 0.730480,
   0.458887, -0.599781, 0.243890,
   0.116971, 0.222313, 0.688396,
   -0.268377, 0.244657, 0.574693,
   0.304252, -0.129121, 0.453988,
   0.100759, -0.433708, 0.282605,
   -0.343713, -0.073814, 0.029226,
   0.251075, 0.083483, 0.238692,
   -0.075623, 0.095008, 0.095425,
   -0.038901, -0.133558, 0.361451,
   -0.226506, 0.315615, 0.008276,
   0.244327, 0.354923, 0.067325,
   0.044735, 0.568618, 0.243966,
   0.119581, -0.446107, 0.097117,
   0.316438, -0.328146, 0.270037,
   0.514750, 0.448266, 0.714832,
   -0.727464, 0.385414, 0.393764,
   0.537968, 0.007156, 0.149009,
   0.450305, 0.004409, 0.105299,
   0.392081, 0.036820, 0.212718,
   -0.095896, 0.592978, 0.065392,
   0.973455, -0.003068, 0.112386,
   0.496669, -0.841329, 0.004186,
   0.441751, -0.163923, 0.489625,
   -0.455431, -0.698782, 0.191856
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

[numthreads(8, 8, 1)]
void main(uint3 dTid : SV_DispatchThreadID, uint2 gTid : SV_GroupThreadID)
{
   float depthSample = depthStencil.GatherRed(gPointClampSampler, dTid.xy / float2(width, height), int2(0, 0));
   if (depthSample == 1.f)
   {
      ssaoOutput[dTid.xy] = 1.f;
      return;
   }
   float3 pos = positionRenderTarget[dTid.xy].xyz;
   float3 normal = normalsRenderTarget[dTid.xy].xyz;
   float3 randomVec = normalize(ssaoNoise[(gTid.x * 8 + gTid.y) % 16]);

   float3 tangent = normalize(randomVec - mul(normal, dot(randomVec, normal)));
   float3 biTangent = mul(normal, tangent);
   float3x3 TBN = float3x3(tangent, biTangent, normal);

   float radius = 0.25f;
   float bias = 0.0025f;
   uint kernel = 64;

   float occlusion = 0.f;
   for (int i = 0; i < kernel; ++i)
   {
      float3 sampleTap = mul(TBN, ssaoKernel[i]);
      sampleTap = pos + sampleTap * radius;

      float4 offset = mul(projMatrix, float4(sampleTap, 1.f));
      offset.xyz /= offset.w;
      offset.xy = offset.xy * 0.5 + 0.5;
      offset.y = 1.f - offset.y;
      float depth = positionRenderTarget.GatherBlue(gPointClampSampler, offset.xy);
      float rangeCheck = smoothstep(0.0, 1.0, radius / abs(pos.z - depth));
      occlusion += (depth >= sampleTap.z + bias ? 1.0 : 0.0) * rangeCheck;
   }
   occlusion = occlusion / float(kernel);

   ssaoOutput[dTid.xy] = occlusion;
}
