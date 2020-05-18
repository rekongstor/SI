#pragma once
#include <vector>

class Elements
{
   std::vector<std::vector<float>> arrays;
   size_t size;
   size_t partitions;

public:
   Elements(size_t size, size_t partitions);
   void execute();
   // there could be getElement(.) or something like that
};

