struct VSInput
{
   float4 position : POSITION;
   float4 normal : NORMAL;
   float2 uv : TEXCOORD;
};

struct PSInput
{
   float4 position : SV_POSITION;
   float4 pos : POSITION;
   float4 normal : NORMAL0;
   float2 uv : TEXCOORD;
};

struct InstanceData
{
   float4x4 world;
   float4x4 worldIt;
};

cbuffer cbPass : register(b0)
{
float4x4 viewMatrix;
float4x4 projMatrix;
}

StructuredBuffer<InstanceData> gInstanceData : register(t0, space0);

PSInput main(VSInput input, uint instanceID : SV_InstanceID)
{
   PSInput output;
   InstanceData inst = gInstanceData[instanceID];

   output.pos = mul(viewMatrix, mul(inst.world, input.position));
   output.position = mul(projMatrix, output.pos);
   output.pos /= output.pos.w;
   output.normal = normalize(mul(viewMatrix, mul(inst.worldIt, input.normal)));
   output.uv = input.uv;
   return output;
}
