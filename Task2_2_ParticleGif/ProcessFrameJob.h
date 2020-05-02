#pragma once
#include "BasicJob.h"
#include "mpmc_bounded_queue.h"
#include "ParticleFrame.h"

class ProcessFrameJob :
   public BasicJob
{
   mpmc_bounded_queue<ParticleFrame>* framesQueue;
public:
   ProcessFrameJob(mpmc_bounded_queue<ParticleFrame>* framesQueue);
   ProcessFrameJob();
   void Run() override;
   void RunAsync() override;
   void Wait() override;
};

