#include "GenerationJob.h"
#include <cmath>

GenerationJob::GenerationJob(std::vector<float>* array, size_t size): array(array), size(size)
{
}

GenerationJob::GenerationJob()
{
}


void GenerationJob::Run()
{
   // Allocating memory
   (*array).resize(size);

   // Generating elements
   for (auto i = 0; i < size; ++i)
      (*array)[i] = powf(sinf(i), cosf(i));
}

void GenerationJob::RunAsync()
{
   job = std::thread([&]() { this->Run(); });
}

void GenerationJob::Wait()
{
   job.join();
}
