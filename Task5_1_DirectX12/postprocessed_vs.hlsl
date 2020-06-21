struct inputVertex
{
   float4 position : POSITION;
   float4 normal : NORMAL;
   float2 uv : TEXCOORD;
};

struct outputVertex
{
   float4 position : SV_POSITION;
   float2 uv : TEXCOORD;
};

outputVertex main(inputVertex input)
{
   outputVertex output;
   output.position = input.position;
   output.uv = input.uv;
   return output;
}