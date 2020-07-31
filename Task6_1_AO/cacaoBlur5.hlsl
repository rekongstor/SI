#include "cacaoBlur.hlsl"

[numthreads(BLUR_WIDTH, BLUR_HEIGHT, 1)]
void main(uint2 tid : SV_GroupThreadID, uint2 gid : SV_GroupID)
{
	LDSEdgeSensitiveBlur(5, tid, gid);
}