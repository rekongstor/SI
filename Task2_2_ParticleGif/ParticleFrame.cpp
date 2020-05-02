#include "ParticleFrame.h"
#include <algorithm>
#include <thread>

ParticleFrame::ParticleFrame(const std::vector<Particle>* particles, int time, uint8_t* pixels) :
   particles(particles),
   time(time),
   pixels(pixels)
{
}

ParticleFrame::ParticleFrame()
{
}


void ParticleFrame::Process()
{
   auto setPixel = [&](size_t posX, size_t posY, size_t offset, float value)
   {
      size_t index = posY * 64 * 4 + posX * 4 + offset;
      pixels[index] = std::clamp(value * 255.f + static_cast<float>(pixels[index]), 0.f, 255.f);
   };

   for (auto& p : *particles)
   {
      long posX = 32 + p.velocityInit.x * time - p.velocityInit.x * p.velocityFade * time * time;
      long posY = 32 + p.velocityInit.y * time - p.velocityInit.y * p.velocityFade * time * time;
      if (posX > 63 || posY > 63 ||
         posX < 0 || posY < 0)
         continue;

      float colorRed = std::clamp(p.colorInit.r - p.colorInit.r * p.colorFade * static_cast<float>(time), 0.f, 1.f);
      float colorGreen = std::clamp(p.colorInit.g - p.colorInit.g * p.colorFade * static_cast<float>(time), 0.f, 1.f);
      float colorBlue = std::clamp(p.colorInit.b - p.colorInit.b * p.colorFade * static_cast<float>(time), 0.f, 1.f);

      setPixel(posX, posY, 0, colorRed);
      setPixel(posX, posY, 1, colorGreen);
      setPixel(posX, posY, 2, colorBlue);
   }
}
