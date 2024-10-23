#pragma once

namespace NR
{
	class Noise
	{
	public:
		static float PerlinNoise(float x, float y);
		static std::array<glm::vec4, 16> HBAOJitter();
	};
}