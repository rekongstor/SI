#pragma once
#include "../Core/BasicJob.h"
#include "mpmc_bounded_queue.hpp"
#include "ParticleFrame.h"

class ProcessFrameJob :
   public BasicJob
{
   mpmc_bounded_queue<ParticleFrame>* framesQueue;
public:
   ProcessFrameJob(mpmc_bounded_queue<ParticleFrame>* framesQueue);
   ProcessFrameJob();
   void Run() override;
};

