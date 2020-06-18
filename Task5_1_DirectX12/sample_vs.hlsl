struct inputVertex
{
   float4 position : POSITION;
   float4 color : COLOR;
};

struct outputVertex
{
   float4 position : SV_POSITION;
   float4 color : COLOR;
};

cbuffer cbPass : register(b0)
{
   float4x4 wvpMat;
}

struct InstanceData
{
   float4x4 world;
};
StructuredBuffer<InstanceData> gInstanceData : register(t0, space1);

outputVertex main(inputVertex input, uint instanceID : SV_InstanceID)
{
   outputVertex output;
   InstanceData instanceData = gInstanceData[instanceID];
   output.position = mul(input.position, instanceData.world);
   output.position = output.position;
   output.color = input.color;
   return output;
}
