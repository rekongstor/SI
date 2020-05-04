#pragma once
#include <thread>

class BasicJob
{
protected:
   std::thread job;
public:
   virtual void Run() = 0;
   void RunAsync();
   void Wait();

   virtual ~BasicJob() = default;
};
