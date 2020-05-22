#include "ParticleSequenceJob.h"

#define INIT_VELOCITY 20.f
#define VELOCITY_FADE 0.1f
#define MIN_COLOR {192,32,0}
#define MAX_COLOR {255,128,8}
#define COLOR_FADE 0.1f

#define SIZE 64
#define TIME 600

int main()
{
   std::pair<int, const char*> particleSequences[] =
   {
      {100, "ps100.gif"},
      {1000, "ps1000.gif"},
      {10000, "ps10000.gif"},
      {1010000000, "ps1000000.gif"}
   };
   for (auto& ps : particleSequences)
   {
      auto [count, filename] = ps;
      ParticleSequence sequence(count, SIZE, TIME);
      sequence.init(MIN_COLOR, MAX_COLOR, INIT_VELOCITY, VELOCITY_FADE, COLOR_FADE);
      ParticleSequenceJob sequenceJob(sequence, filename);
      sequenceJob.Run();
   }
}
