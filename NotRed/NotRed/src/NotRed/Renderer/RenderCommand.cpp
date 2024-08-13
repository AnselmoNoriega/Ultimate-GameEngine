#include "nrpch.h"
#include "RenderCommand.h"

#include "RendererAPI.h"

namespace NR
{
	uint32_t RenderCommand::Clear(void* datablock)
	{
		float* data = (float*)datablock;

		float r = *data++;
		float g = *data++;
		float b = *data++;
		float a = *data;

		RendererAPI::Clear(r, g, b, a);

		return sizeof(float) * 4;
	}
}