struct outputVertex
{
   float4 position : SV_POSITION;
   float4 color : COLOR;
};


float4 main(outputVertex vertex) : SV_TARGET
{
   return float4(vertex.color.x, vertex.color.y, vertex.color.z, 1.0f);
}
