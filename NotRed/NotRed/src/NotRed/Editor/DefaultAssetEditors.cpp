#include "nrpch.h"
#include "DefaultAssetEditors.h"

#include "NotRed/Asset/AssetImporter.h"

namespace NR
{
	PhysicsMaterialEditor::PhysicsMaterialEditor()
		: AssetEditor("Edit Physics Material") {}

	void PhysicsMaterialEditor::Close()
	{
		AssetImporter::Serialize(mAsset);
		mAsset = nullptr;
	}

	void PhysicsMaterialEditor::Render()
	{
		if (!mAsset)
		{
			SetOpen(false);
		}

		UI::BeginPropertyGrid();
		UI::Property("Static friction", mAsset->StaticFriction);
		UI::Property("Dynamic friction", mAsset->DynamicFriction);
		UI::Property("Bounciness", mAsset->Bounciness);
		UI::EndPropertyGrid();
	}

	TextureViewer::TextureViewer()
		: AssetEditor("Edit Texture")
	{
		SetMinSize(200, 600);
		SetMaxSize(500, 1000);
	}

	void TextureViewer::Close()
	{
		mAsset = nullptr;
	}

	void TextureViewer::Render()
	{
		if (!mAsset)
		{
			SetOpen(false);
		}

		float textureWidth = mAsset->GetWidth();
		float textureHeight = mAsset->GetHeight();
		float imageSize = ImGui::GetWindowWidth() - 40;
		imageSize = glm::min(imageSize, 500.0f);

		ImGui::SetCursorPosX(20);

		UI::BeginPropertyGrid();		
		UI::Property("Width", textureWidth, 0.1f, 0.0f, 0.0f, true);
		UI::Property("Height", textureHeight, 0.1f, 0.0f, 0.0f, true);
		UI::EndPropertyGrid();
	}

}