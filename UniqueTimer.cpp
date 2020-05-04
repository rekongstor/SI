#include "UniqueTimer.h"

UniqueTimer::UniqueTimer(const char* text)
{
   std::cout << text << std::endl;
}


UniqueTimer::~UniqueTimer()
{
   std::cout << "Time elapsed: " << std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::high_resolution_clock::now() - start).count() << std::endl;
}
