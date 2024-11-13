#pragma once

#include "NotRed/Core/Ref.h"

struct GLFWwindow;

namespace NR
{
	class RendererContext : public RefCounted
	{
	public:
		RendererContext() = default;
		virtual ~RendererContext() = default;

		virtual void Init() = 0;

		static Ref<RendererContext> Create();
	};
}