#pragma once
#include <vector>
#include "../Core/BasicJob.h"

class GenerationJob :
   public BasicJob
{
   std::vector<float>* array;
   size_t size;
public:
   GenerationJob(std::vector<float>* array, size_t size);
   GenerationJob();

   void Run() override;
};
