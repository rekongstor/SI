#include "ParticleSequence.h"

#define INIT_VELOCITY 20.f
#define VELOCITY_FADE 0.1f
#define MIN_COLOR {192,32,0}
#define MAX_COLOR {255,128,8}
#define COLOR_FADE 0.1f

int main()
{
   ParticleSequence ps100(100);
   ps100.execute(MIN_COLOR, MAX_COLOR, INIT_VELOCITY, VELOCITY_FADE, COLOR_FADE, 1, "ps100.gif");
   ParticleSequence ps1000(1000);
   ps1000.execute(MIN_COLOR, MAX_COLOR, INIT_VELOCITY, VELOCITY_FADE, COLOR_FADE, 1, "ps1000.gif");
   ParticleSequence ps10000(10000);
   ps10000.execute(MIN_COLOR, MAX_COLOR, INIT_VELOCITY, VELOCITY_FADE, COLOR_FADE, 1, "ps10000.gif");
   ParticleSequence ps1000000(1000000);
   ps1000000.execute(MIN_COLOR, MAX_COLOR, INIT_VELOCITY, VELOCITY_FADE, COLOR_FADE, 1, "ps1000000.gif");
}
