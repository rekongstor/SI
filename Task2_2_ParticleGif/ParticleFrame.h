#pragma once
#include <vector>
#include "Particle.h"

class ParticleFrame
{
	std::vector<uint8_t> pixels;
public:
	ParticleFrame(const std::vector<Particle>& particles, int time);
	const uint8_t* data() const;
};
