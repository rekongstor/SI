#include "ProcessFrameJob.h"

ProcessFrameJob::ProcessFrameJob(mpmc_bounded_queue<ParticleFrame>* framesQueue):
   framesQueue(framesQueue)
{
}

ProcessFrameJob::ProcessFrameJob()
{
}


void ProcessFrameJob::Run()
{
   ParticleFrame particleFrame;
   while ((*framesQueue).dequeue(particleFrame))
   {
      particleFrame.Process();
   }
}

void ProcessFrameJob::RunAsync()
{
   job = std::thread([&]()
      {
         Run();
      });
}

void ProcessFrameJob::Wait()
{
   job.join();
}
