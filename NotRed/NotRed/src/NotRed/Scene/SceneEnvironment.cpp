#pragma once

#include "SceneEnviroment.h"

namespace NR
{
	Environment::Environment(const Ref<TextureCube>& radianceMap, const Ref<TextureCube>& irradianceMap)
		: RadianceMap(radianceMap), IrradianceMap(irradianceMap) 
	{

	}
}