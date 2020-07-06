#pragma once
#include <chrono>

class siTimer
{
   std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
public:
   float delta();
};

