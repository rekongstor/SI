struct outputVertex
{
   float4 position : SV_POSITION;
   float4 color : COLOR0;
   float4 normal : NORMAL;
   float4 material : COLOR1;
};

cbuffer cbPass : register(b0)
{
   float4 cbDirection;
   float4 cbColor;
   float4x4 viewProj;
   float4 ambientColor;
}


float4 main(outputVertex input) : SV_TARGET
{
   float4 output = input.material * (ambientColor + cbColor * max(dot(input.normal, -cbDirection), 0.f));
   return output;
}
