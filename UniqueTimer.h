#pragma once
#include <chrono>
#include <iostream>

class UniqueTimer
{
   std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
public:
   UniqueTimer(const char* text);
   ~UniqueTimer();
};
