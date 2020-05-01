#pragma once
#include "ParticleFrame.h"
#include "Particle.h"
#include <vector>

class ParticleSequence
{
	struct MAX_COLOR_VAL
	{
		uint8_t red;
		uint8_t green;
		uint8_t blue;
	};

	std::vector<ParticleFrame> frames;
	std::vector<Particle> particles;
public:
	ParticleSequence(MAX_COLOR_VAL minColorValRGB, MAX_COLOR_VAL maxColorValRGB, float initialVelocity,
	                 float velocityFade, float colorFade, uint32_t particleCount, uint32_t delay,
	                 const char* filename = "particle_sequence.gif");
};
