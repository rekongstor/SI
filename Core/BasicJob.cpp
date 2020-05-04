#include "BasicJob.h"


void BasicJob::RunAsync()
{
   job = std::thread([&]()
      {
         Run();
      });
}

void BasicJob::Wait()
{
   job.join();
}
