#include "Elements.h"

#include "GenerationJob.h"
#include "../Core/UniqueTimer.h"


Elements::Elements(size_t size, size_t partitions)
{
   UniqueTimer timer(" parallel jobs");

   // We suggest that we need to perform (partitions) different memory allocations (or vectors)
   arrays.resize(partitions);

   // Each vector represents its job
   std::vector<std::shared_ptr<BasicJob>> jobs;

   // Creating jobs
   for (int i = 0; i < partitions - 1; ++i)
   {
      jobs.emplace_back(new GenerationJob(&arrays[i], size / partitions));
      jobs.back()->RunAsync();
   }

   // Creating job for (size / partition) + left elements
   GenerationJob(&arrays[partitions - 1], size / partitions + size % partitions).Run();

   // Wait for jobs to end
   for (auto& job : jobs)
      job->Wait();
}
