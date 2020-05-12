#include <random>
#include <string>

#define SIZE 64

// TIME <= QUEUE_SIZE
#define TIME 600
#define QUEUE_SIZE 1024

#include "ParticleSequence.h"
#include "ProcessFrameJob.h"
#include "GifWriterJob.h"
#include "../Core/UniqueTimer.h"

void ParticleSequence::execute(ParticleSequence::MAX_COLOR_VAL minColorValRGB,
                               ParticleSequence::MAX_COLOR_VAL maxColorValRGB, float initialVelocity,
                               float velocityFade, float colorFade, uint32_t delay, const char* filename)
{
   init(minColorValRGB, maxColorValRGB, initialVelocity, velocityFade, colorFade, filename);

   runWork(delay, filename);
}

ParticleSequence::ParticleSequence(uint32_t particleCount) :
   particles(particleCount),
   framesQueue(QUEUE_SIZE),
   framesData(SIZE * SIZE * 4 * TIME)
{
}

void ParticleSequence::runWork(uint32_t delay, const char* filename)
{
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


void ParticleSequence::init(MAX_COLOR_VAL minColorValRGB, MAX_COLOR_VAL maxColorValRGB, float initialVelocity,
                            float velocityFade, float colorFade, const char* filename)
{
   UniqueTimer ut(filename);
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
