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

cbuffer ConstantBuffer : register(b0)
{
   float4x4 wvpMat;
}

outputVertex main(inputVertex input)
{
   outputVertex output;
   output.position = mul(input.position, wvpMat);
   output.position = output.position / output.position.w;
   output.color = input.color;
   return output;
}
