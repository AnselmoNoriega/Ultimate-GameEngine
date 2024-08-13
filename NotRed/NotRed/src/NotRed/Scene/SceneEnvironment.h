#pragma once

#include "NotRed/Renderer/Texture.h"

namespace NR
{
	class Environment
	{
	public:
		Environment() = default;
		Environment(const Ref<TextureCube>& radianceMap, const Ref<TextureCube>& irradianceMap);

	public:
		Ref<TextureCube> RadianceMap;
		Ref<TextureCube> IrradianceMap;
	};
}