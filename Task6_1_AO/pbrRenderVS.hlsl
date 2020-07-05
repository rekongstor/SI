struct VSInput
{
   float4 pos : POSITION;
   float4 normal : NORMAL;
   float2 uv : TEXCOORD;
};

struct PSInput
{
   float4 position : SV_POSITION;
   float4 color : COLOR0;
   //float4 normal : NORMAL0;
   //float4 material : COLOR1;
   //float4 view : NORMAL1;
   //float2 uv : TEXCOORD;
};

PSInput main(VSInput input)
{
   PSInput output;
   output.position = input.pos;
   output.color = input.pos;
	return output;
}