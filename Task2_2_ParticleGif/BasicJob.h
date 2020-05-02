#pragma once
#include <thread>

class BasicJob
{
protected:
   std::thread job;
public:
   virtual void Run() = 0;
   virtual void RunAsync() = 0;
   virtual void Wait() = 0;

   virtual ~BasicJob() = default;
};
