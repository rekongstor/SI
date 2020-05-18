#pragma once
#include <vector>
#include "mpmc_bounded_queue.hpp"
#include "Particle.h"

class ParticleSequenceJob;

class ParticleSequence
{
   friend class ParticleSequenceJob;
   struct COLOR_VAL
   {
      uint8_t red;
      uint8_t green;
      uint8_t blue;
   };

   size_t time;
   std::vector<uint8_t> framesData;
   std::vector<Particle> particles;
public:
   ParticleSequence(uint32_t particleCount, uint32_t size, size_t time);
   void init(COLOR_VAL minColorValRGB, COLOR_VAL maxColorValRGB, float initialVelocity,
             float velocityFade, float colorFade);
};
