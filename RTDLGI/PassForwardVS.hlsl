

struct Vertex
{
    float3 position : POSITION;
    float3 normal : NORMAL0;
};

float4 main( Vertex v ) : SV_POSITION
{
	return float4(v.position, 1.f);
}