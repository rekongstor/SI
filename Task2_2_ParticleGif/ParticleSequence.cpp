#include <random>
#include <string>

#include "ParticleSequence.h"
#include "ProcessFrameJob.h"
#include "GifWriterJob.h"
#include "UniqueTimer.h"

ParticleSequence::ParticleSequence(MAX_COLOR_VAL minColorValRGB, MAX_COLOR_VAL maxColorValRGB, float initialVelocity,
                                   float velocityFade, float colorFade, uint32_t particleCount, uint32_t delay,
                                   const char* filename) :
   particles(particleCount),
   framesQueue(1024),
   framesData(64 * 64 * 4 * 600)
{
   UniqueTimer ut(filename);
   // Creating RNG
   std::mt19937 engine(4221);
   std::normal_distribution<float> distNormal;
   const std::uniform_real_distribution<float> distUniform;

   // Creating Particles
   for (auto& p : particles)
   {
      p.velocityInit = {distNormal(engine) * initialVelocity, distNormal(engine) * initialVelocity};
      p.velocityFade = distUniform(engine) * velocityFade;
      p.colorInit = {
         minColorValRGB.red + distUniform(engine) * (maxColorValRGB.red - minColorValRGB.red),
         minColorValRGB.green + distUniform(engine) * (maxColorValRGB.green - minColorValRGB.green),
         minColorValRGB.blue + distUniform(engine) * (maxColorValRGB.blue - minColorValRGB.blue)
      };
      p.colorFade = distUniform(engine) * colorFade;
   }

   // Enqueue frames
   for (int i = 0; i < 600; ++i)
   {
      framesQueue.enqueue(ParticleFrame(&particles, i, &(framesData.data()[i * 64 * 64 * 4])));
   }

   // Create jobs to run synchronously
   std::vector<std::shared_ptr<ProcessFrameJob>> processFrameJobs;
   processFrameJobs.reserve(std::thread::hardware_concurrency() - 1);
   for (int i = 0; i < std::thread::hardware_concurrency() - 1; ++i)
      processFrameJobs.emplace_back(new ProcessFrameJob(&framesQueue));

   // And create job for the main thread
   ProcessFrameJob processFrameJob(&framesQueue);

   // Create GifWriterJob
   GifWriterJob gifWriterJob(filename, delay, framesData);

   // Falls to sleep immediately
   gifWriterJob.RunAsync();

   // Run 
   for (int i = 0; i < std::thread::hardware_concurrency() - 1; ++i)
      processFrameJobs[i]->RunAsync();
   processFrameJob.Run();

   // Wait for others
   for (int i = 0; i < std::thread::hardware_concurrency() - 1; ++i)
      processFrameJobs[i]->Wait();

   gifWriterJob.Notify();
   gifWriterJob.Wait();
}
