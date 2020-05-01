#include "ParticleSequence.h"
#include <vector>

int main()
{
	ParticleSequence ps100({188, 0, 0}, {255, 8, 0}, 1.5f, 0.001f, .1f, 100, 1, "ps100.gif");
	ParticleSequence ps1000({188, 0, 0}, {255, 8, 0}, 1.5f, 0.001f, .1f, 1000, 1, "ps1000.gif");
	ParticleSequence ps10000({188, 0, 0}, {255, 8, 0}, 1.5f, 0.001f, .1f, 10000, 1, "ps10000.gif");
}
