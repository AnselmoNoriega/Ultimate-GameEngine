#pragma once

#include "NotRed/Scene/Scene.h"
#include "NotRed/Scene/Entity.h"

namespace NR
{
	class ECSPanel
	{
	public:
		ECSPanel(Ref<Scene> context);
		~ECSPanel();

		void OnImGuiRender(bool& open);

		void SetContext(Ref<Scene> context) { mContext = context; }
		void SetSelected(Entity entity) { mSelectedEntity = entity; }
	private:
		Ref<Scene> mContext;
		Entity mSelectedEntity;
	};
}