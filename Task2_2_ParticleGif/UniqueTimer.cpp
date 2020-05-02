#include "UniqueTimer.h"

UniqueTimer::UniqueTimer(std::string_view stringView)
{
   std::cout << stringView << std::endl;
}

UniqueTimer::~UniqueTimer()
{
   std::cout << "Time elapsed: " << std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::high_resolution_clock::now() - start).count() << std::endl;
}
