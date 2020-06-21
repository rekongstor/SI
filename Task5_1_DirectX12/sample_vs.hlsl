struct inputVertex
{
   float4 position : POSITION;
   float4 normal : NORMAL;
   float2 uv : TEXCOORD;
};

struct outputVertex
{
   float4 position : SV_POSITION;
   float4 color : COLOR0;
   float4 normal : NORMAL0;
   float4 material : COLOR1;
   float4 view : NORMAL1;
   float2 uv : TEXCOORD;
};

struct InstanceData
{
   float4x4 world;
   float4 material;
};


cbuffer cbPass : register(b0)
{
   float4 cbDirection;
   float4 cbColor;
   float4x4 viewProj;
   float4 ambientColor;
   float4 camPos;
}

StructuredBuffer<InstanceData> gInstanceData : register(t0, space0);

outputVertex main(inputVertex input, uint instanceID : SV_InstanceID)
{
   outputVertex output;
   InstanceData instanceData = gInstanceData[instanceID];
   output.position = mul(input.position, instanceData.world);
   output.position = mul(output.position, viewProj);
   output.color = float4(1.f,0.f,0.f,1.f);
   output.normal = normalize(mul(input.normal, instanceData.world));
   output.view = camPos - input.position;
   output.material = instanceData.material;
   output.uv = input.uv;
   return output;
}
