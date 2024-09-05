#include "nrpch.h"
#include "Noise.h"

#include "FastNoise.h"

namespace NR
{
	static FastNoise sFastNoise;

	float Noise::PerlinNoise(float x, float y)
	{
		sFastNoise.SetNoiseType(FastNoise::Perlin);
		float result = sFastNoise.GetNoise(x, y); // This returns a value between -1 and 1
		return result;
	}
}