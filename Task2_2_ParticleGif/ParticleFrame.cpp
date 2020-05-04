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
   // Lambda sets pixel value between 0 and 255
   auto setPixel = [&](size_t posX, size_t posY, size_t offset, float value)
   {
      size_t index = posY * 64 * 4 + posX * 4 + offset;
      pixels[index] = std::clamp(value * 255.f + static_cast<float>(pixels[index]), 0.f, 255.f);
   };

   // Processing all particles
   for (auto& p : *particles)
   {
      // Calculating fading for distance (sigmoid function)
      float distanceFade = p.velocityFade * static_cast<float>(time) / (1 + 0.2f * abs(
         p.velocityFade * static_cast<float>(time)));

      // Calculating fading for color (sigmoid function)
      float colorFade = p.colorFade * static_cast<float>(time) / (1 + abs(p.colorFade * static_cast<float>(time)));

      // Calculating pixel position
      long posX = 32 + p.velocityInit.x * distanceFade * 32;
      long posY = 32 + p.velocityInit.y * distanceFade * 32;

      // Scissoring
      if (posX > 63 || posY > 63 ||
         posX < 0 || posY < 0)
         continue;

      // Calculating particle color
      float colorRed = std::clamp(p.colorInit.r * (1.f - colorFade) / 255.f, 0.f, 1.f);
      float colorGreen = std::clamp(p.colorInit.g * (1.f - colorFade) / 255.f, 0.f, 1.f);
      float colorBlue = std::clamp(p.colorInit.b * (1.f - colorFade) / 255.f, 0.f, 1.f);

      // Setting pixels
      setPixel(posX, posY, 0, colorRed);
      setPixel(posX, posY, 1, colorGreen);
      setPixel(posX, posY, 2, colorBlue);
   }
}
