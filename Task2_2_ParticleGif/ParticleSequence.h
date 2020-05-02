#pragma once
#include <vector>

#include "mpmc_bounded_queue.h"
#include "Particle.h"
#include "ParticleFrame.h"

class ParticleSequence
{
	struct MAX_COLOR_VAL
	{
		uint8_t red;
		uint8_t green;
		uint8_t blue;
	};

	mpmc_bounded_queue<ParticleFrame> framesQueue;
	std::vector<uint8_t> framesData;
	std::vector<Particle> particles;
public:
	ParticleSequence(MAX_COLOR_VAL minColorValRGB, MAX_COLOR_VAL maxColorValRGB, float initialVelocity,
	                 float velocityFade, float colorFade, uint32_t particleCount, uint32_t delay,
	                 const char* filename = "particle_sequence.gif");
};
