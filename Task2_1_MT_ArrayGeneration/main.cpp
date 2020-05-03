#include <iostream>

#include "Elements.h"

#define n 1000000

int main()
{
   for (size_t i = 0; i < 32; ++i)
   {
      std::cout << "Generating array with " << i + 1;
      Elements elements(n, i + 1);
   }
   return 0;
}
