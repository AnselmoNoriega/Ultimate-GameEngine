#include "nrpch.h"
#include "Noise.h"

#include "FastNoise.h"

namespace NR
{
	static FastNoise sFastNoise;
	static std::uniform_real_distribution<float> sJitters(0.f, 1.f);
	static std::default_random_engine sJitterGenerator(1337u);

	float Noise::PerlinNoise(float x, float y)
	{
		sFastNoise.SetNoiseType(FastNoise::Perlin);
		float result = sFastNoise.GetNoise(x, y); // This returns a value between -1 and 1
		return result;
	}

	std::array<glm::vec4, 16> Noise::HBAOJitter()
	{
		constexpr float PI = 3.14159265358979323846264338f;
		const float numDir = 8.f;  // keep in sync to glsl
		std::array<glm::vec4, 16> result{};
		for (int i = 0; i < 16; i++)
		{
			float Rand1 = sJitters(sJitterGenerator);
			float Rand2 = sJitters(sJitterGenerator);

			// Use random rotation angles in [0,2PI/NUM_DIRECTIONS)
			const float Angle = 2.f * PI * Rand1 / numDir;
			result[i].x = cosf(Angle);
			result[i].y = sinf(Angle);
			result[i].z = Rand2;
			result[i].w = 0;
		}
		return result;
	}
}