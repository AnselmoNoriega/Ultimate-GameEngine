#pragma once

#include "NotRed/Core/Ref.h"
#include "NotRed/Scene/Scene.h"
#include "NotRed/Project/Project.h"
#include "NotRed/Core/Events/Event.h"

namespace NR
{
	class EditorPanel : public RefCounted
	{
	public:
		virtual void ImGuiRender(bool& isOpen) = 0;
		virtual void OnEvent(Event& e) {}
		virtual void ProjectChanged(const Ref<Project>& project) {}
		virtual void SetSceneContext(const Ref<Scene>& context) {}
	};
}