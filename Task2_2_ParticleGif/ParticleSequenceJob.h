#pragma once
#include "ParticleFrame.h"
#include "ParticleSequence.h"
#include "../Core/BasicJob.h"

class ParticleSequenceJob : public BasicJob 
{
   ParticleSequence& particleSequence;
   const char* filename;
   uint32_t delay;
   mpmc_bounded_queue<ParticleFrame> framesQueue;
   static size_t alignBufferSize(size_t time);
public:
   ParticleSequenceJob(ParticleSequence& particleSequence, const char* filename, uint32_t delay = 1);

   void Run() override;
};

