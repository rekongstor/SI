struct outputVertex
{
   float4 position : SV_POSITION;
   float4 color : COLOR;
   float4 normal : NORMAL;
};

cbuffer cbPass : register(b0)
{
   float4 cbDirection;
   float4 cbColor;
}


float4 main(outputVertex input) : SV_TARGET
{
   float4 output = input.color * cbColor * max(dot(input.normal, -cbDirection), 0.f);
   return output;
}
