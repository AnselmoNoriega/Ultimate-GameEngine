#include "nrpch.h"
#include "ECSPanel.h"

#include "NotRed/ImGui/ImGui.h"

namespace NR
{
	ECSPanel::ECSPanel(Ref<Scene> context)
		: mContext(context)
	{
	}

	ECSPanel::~ECSPanel()
	{
	}

	void ECSPanel::ImGuiRender(bool& open)
	{
		if (!open)
		{
			return;
		}

		ImGui::Begin("ECS Debug", &open);

		if (mContext)
		{
			for (auto e : mContext->mRegistry.view<IDComponent>())
			{
				Entity entity = { e, mContext.Raw() };
				const auto& name = entity.Name();

				std::string label = fmt::format("{0} - {1}", name, entity.GetID());

				bool selected = mSelectedEntity == entity;
				if (ImGui::Selectable(label.c_str(), &selected, 0))
				{
					mSelectedEntity = entity;
				}
			}

		}

		ImGui::End();
	}

}