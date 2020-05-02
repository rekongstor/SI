#pragma once
#include <vector>
#include "Particle.h"
#include "BasicJob.h"

class ParticleFrame
{
   const std::vector<Particle>* particles;
   int time;
   uint8_t* pixels;

public:
   ParticleFrame(const std::vector<Particle>* particles, int time, uint8_t* pixels);
   ParticleFrame();

   void Process();
};
