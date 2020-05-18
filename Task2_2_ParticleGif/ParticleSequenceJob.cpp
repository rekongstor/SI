#include "ParticleSequenceJob.h"
#include "GifWriterJob.h"
#include "ProcessFrameJob.h"
#include "../Core/UniqueTimer.h"

size_t ParticleSequenceJob::alignBufferSize(size_t time)
{
   uint8_t i;
   for (i = 0; time != 0; ++i)
      time >>= 1;
   return 1 << i;
}

ParticleSequenceJob::ParticleSequenceJob(ParticleSequence& particleSequence, const char* filename, uint32_t delay)
   : particleSequence(particleSequence),
     filename(filename),
     delay(delay),
     framesQueue(alignBufferSize(particleSequence.time))
{
}

void ParticleSequenceJob::Run()
{
   UniqueTimer ut(filename);
   // Enqueue frames
   for (int i = 0; i < 600; ++i)
   {
      framesQueue.enqueue(ParticleFrame(&particleSequence.particles, i,
                                        &(particleSequence.framesData.data()[i * 64 * 64 * 4])));
   }

   // Create jobs to run synchronously
   std::vector<std::shared_ptr<ProcessFrameJob>> processFrameJobs;
   processFrameJobs.reserve(std::thread::hardware_concurrency() - 1);
   for (int i = 0; i < std::thread::hardware_concurrency() - 1; ++i)
      processFrameJobs.emplace_back(new ProcessFrameJob(&framesQueue));

   // And create job for the main thread
   ProcessFrameJob processFrameJob(&framesQueue);

   // Create GifWriterJob
   GifWriterJob gifWriterJob(filename, delay, particleSequence.framesData);

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
