#pragma once
#include <condition_variable>
#include <vector>

#include "BasicJob.h"

class GifWriterJob : public BasicJob
{
   const char* filename;
   uint32_t delay;
   std::vector<uint8_t>& framesData;

   std::mutex mutexCV;
   std::condition_variable conditionVariable;
   bool ready = false;
public:
   GifWriterJob(const char* filename, uint32_t delay, std::vector<uint8_t>& framesData);
   void Notify();

   void Run() override;
   void RunAsync() override;
   void Wait() override;
};
