#include "ParticleSequence.h"

#define INIT_VELOCITY 20.f
#define VELOCITY_FADE 0.1f
#define MIN_COLOR {192,32,0}
#define MAX_COLOR {255,128,8}
#define COLOR_FADE 0.1f

int main()
{
   ParticleSequence ps100
      (MIN_COLOR, MAX_COLOR, INIT_VELOCITY, VELOCITY_FADE, COLOR_FADE, 100, 1, "ps100.gif");
   ParticleSequence ps1000
      (MIN_COLOR, MAX_COLOR, INIT_VELOCITY, VELOCITY_FADE, COLOR_FADE, 1000, 1, "ps1000.gif");
   ParticleSequence ps10000
      (MIN_COLOR, MAX_COLOR, INIT_VELOCITY, VELOCITY_FADE, COLOR_FADE, 10000, 1, "ps10000.gif");
   ParticleSequence ps1000000
      (MIN_COLOR, MAX_COLOR, INIT_VELOCITY, VELOCITY_FADE, COLOR_FADE, 1000000, 1, "ps1000000.gif");
}
