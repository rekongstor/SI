struct inputVertex
{
   float4 position : POSITION;
   float4 color : COLOR;
   float4 normal : NORMAL;
};

struct outputVertex
{
   float4 position : SV_POSITION;
   float4 color : COLOR;
   float4 normal : NORMAL;
};

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
   output.color = input.color;
   output.normal = mul(input.normal, instanceData.world);
   return output;
}
