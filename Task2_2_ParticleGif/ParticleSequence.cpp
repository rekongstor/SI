#include <random>

#include "ParticleSequence.h"
#include "ProcessFrameJob.h"
#include "../Core/UniqueTimer.h"


ParticleSequence::ParticleSequence(uint32_t particleCount, uint32_t size, size_t time) :
   particles(particleCount),
   framesData(size * size * 4 * time),
   time(time)
{
}

void ParticleSequence::init(COLOR_VAL minColorValRGB, COLOR_VAL maxColorValRGB, float initialVelocity,
                            float velocityFade, float colorFade)
{
   // Creating RNG
   std::mt19937 engine(4221);
   std::normal_distribution<float> distNormal;
   std::uniform_real_distribution<float> distUniform;

   // Creating Particles
   for (auto& p : particles)
   {
      p.velocityInit = {distNormal(engine) * initialVelocity, distNormal(engine) * initialVelocity};
      p.velocityFade = abs(distNormal(engine)) * velocityFade;
      p.colorInit = {
         minColorValRGB.red + distUniform(engine) * (maxColorValRGB.red - minColorValRGB.red),
         minColorValRGB.green + distUniform(engine) * (maxColorValRGB.green - minColorValRGB.green),
         minColorValRGB.blue + distUniform(engine) * (maxColorValRGB.blue - minColorValRGB.blue)
      };
      p.colorFade = abs(distNormal(engine)) * colorFade;
   }
}
