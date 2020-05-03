#pragma once
#include <vector>

class Elements
{
   std::vector<std::vector<float>> arrays;


public:
   Elements(size_t size, size_t partitions);
   // there could be getElement(.) or something like that
};

