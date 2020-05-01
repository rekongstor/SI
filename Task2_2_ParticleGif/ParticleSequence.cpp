#include "ParticleSequence.h"
#include "gif-h/gif.h"
#include <random>

ParticleSequence::ParticleSequence(MAX_COLOR_VAL minColorValRGB, MAX_COLOR_VAL maxColorValRGB, float initialVelocity,
                                   float velocityFade, float colorFade, uint32_t particleCount, uint32_t delay,
                                   const char* filename) :
	particles(particleCount)
{
	std::mt19937 engine(4221);
	std::normal_distribution<float> distNormal;
	const std::uniform_real_distribution<float> distUniform;

	for (auto& p : particles)
	{
		p.velocityInit = {distNormal(engine) * initialVelocity, distNormal(engine) * initialVelocity};
		p.velocityFade = distUniform(engine) * velocityFade;
		p.colorInit = {
			minColorValRGB.red + distUniform(engine) * (maxColorValRGB.red - minColorValRGB.red),
			minColorValRGB.green + distUniform(engine) * (maxColorValRGB.green - minColorValRGB.green),
			minColorValRGB.blue + distUniform(engine) * (maxColorValRGB.blue - minColorValRGB.blue)
		};
		p.colorFade = distUniform(engine) * colorFade;
	}

	for (auto i = 0; i < 600; ++i)
		frames.emplace_back(ParticleFrame(particles, i));

	GifWriter gifWriter;

	GifBegin(&gifWriter, filename, 64, 64, delay);
	for (const auto& frame : frames)
		GifWriteFrame(&gifWriter, frame.data(), 64, 64, delay);

	GifEnd(&gifWriter);
}
