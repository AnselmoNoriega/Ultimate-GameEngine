#pragma once

#include "NotRed/Core/Core.h"

namespace NR
{
	class NOT_RED_API Layer
	{
	public:
		Layer(const std::string& name = "Layer");
		virtual ~Layer();

		virtual void Attach() {}
		virtual void Detach() {}
		virtual void Update() {}
		virtual void OnEvent(Event& event) {}

		inline const std::string& GetName() const { return mDebugName; }

	protected:
		std::string mDebugName;
	};
}