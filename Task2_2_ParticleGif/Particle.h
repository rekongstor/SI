#pragma once
struct Particle
{
	struct XY
	{
		float x;
		float y;
	};

	struct RGB
	{
		float r;
		float g;
		float b;
	};

	XY velocityInit;
	float velocityFade;
	RGB colorInit;
	float colorFade;
};
