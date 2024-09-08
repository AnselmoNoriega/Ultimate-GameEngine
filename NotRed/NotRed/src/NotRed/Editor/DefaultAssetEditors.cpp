#include "nrpch.h"
#include "DefaultAssetEditors.h"

namespace NR
{
	PhysicsMaterialEditor::PhysicsMaterialEditor()
		: AssetEditor("Edit Physics Material") {}

	void PhysicsMaterialEditor::Render()
	{
		UI::BeginPropertyGrid();
		UI::Property("Static friction", mAsset->StaticFriction);
		UI::Property("Dynamic friction", mAsset->DynamicFriction);
		UI::Property("Bounciness", mAsset->Bounciness);
		UI::EndPropertyGrid();
	}

	TextureEditor::TextureEditor()
		: AssetEditor("Edit Texture")
	{
		SetMinSize(200, 600);
		SetMaxSize(500, 1000);
	}

	void TextureEditor::Render()
	{
		float textureWidth = mAsset->GetWidth();
		float textureHeight = mAsset->GetHeight();
		float bitsPerPixel = Texture::GetBPP(mAsset->GetFormat());
		float imageSize = ImGui::GetWindowWidth() - 40;
		imageSize = glm::min(imageSize, 500.0f);

		ImGui::SetCursorPosX(20);
		ImGui::Image((ImTextureID)mAsset->GetRendererID(), { imageSize, imageSize });

		UI::BeginPropertyGrid();
		UI::Property("Width", textureWidth, 0.1f, 0.0f, 0.0f, true);
		UI::Property("Height", textureHeight, 0.1f, 0.0f, 0.0f, true);
		UI::Property("Bits", bitsPerPixel, 0.1f, 0.0f, 0.0f, true);
		UI::EndPropertyGrid();
	}

}