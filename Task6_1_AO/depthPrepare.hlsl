Texture2D input;
RWTexture2D<float4> output;

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
   float4 color = input[DTid.xy].xxxx;
   output[DTid.xy] = color;
}