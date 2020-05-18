#pragma once
#include <vector>

#include "mpmc_bounded_queue.hpp"
#include "Particle.h"
#include "ParticleFrame.h"

class ParticleSequence
{
   struct COLOR_VAL
   {
      uint8_t red;
      uint8_t green;
      uint8_t blue;
   };

   mpmc_bounded_queue<ParticleFrame> framesQueue;
   std::vector<uint8_t> framesData;
   std::vector<Particle> particles;
public:
   void execute(COLOR_VAL minColorValRGB, COLOR_VAL maxColorValRGB, float initialVelocity, float velocityFade,
                float colorFade, uint32_t delay, const char* filename);
   ParticleSequence(uint32_t particleCount);

private:
   void init(COLOR_VAL minColorValRGB, COLOR_VAL maxColorValRGB, float initialVelocity,
             float velocityFade, float colorFade, const char* filename = "particle_sequence.gif");

   /*
    * Runs the execution of created frames, writes results in gif file
    */
   void runWork(uint32_t delay, const char* filename);
};
