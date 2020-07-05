struct PSInput
{
   float4 position : SV_POSITION;
   float4 color : COLOR0;
   //float4 normal : NORMAL0;
   //float4 material : COLOR1;
   //float4 view : NORMAL1;
   //float2 uv : TEXCOORD;
};

float4 main(PSInput input) : SV_TARGET
{
	return input.color;
}