struct inputVertex
{
   float3 position : POSITION;
   float3 color : COLOR;
};

struct outputVertex
{
   float4 position : SV_POSITION;
   float4 color : COLOR;
};

cbuffer ConstantBuffer : register(b0)
{
   float4 colorMultiplier;
}

outputVertex main(inputVertex input)
{
   outputVertex output;
   output.position = float4(input.position, 1.f);
   output.color = float4(input.color, 1.f) * colorMultiplier;
   return output;
}
