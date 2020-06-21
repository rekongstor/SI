cbuffer cbSettings
{
};


Texture2D input;
RWTexture2D<float4> output;

[numthreads(256, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{

   float4 color = input[DTid.xy];
   color = color / (color + 1.f);
   output[DTid.xy] = pow(color, 1.f / 2.2f);
}