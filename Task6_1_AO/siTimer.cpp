#include "siTimer.h"

float siTimer::delta()
{
   std::chrono::system_clock::time_point nowTime = std::chrono::system_clock::now();
   float delta = static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(nowTime - startTime).count());
   startTime = nowTime;
   return delta;
}
