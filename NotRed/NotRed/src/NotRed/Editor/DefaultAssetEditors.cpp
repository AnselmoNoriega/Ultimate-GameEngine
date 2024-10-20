#include "nrpch.h"
#include "DefaultAssetEditors.h"

#include "NotRed/Asset/AssetImporter.h"
#include "NotRed/Asset/AssetManager.h"
#include "NotRed/Audio/AudioFileUtils.h"
#include "NotRed/Audio/Sound.h"

#include "imgui_internal.h"

namespace NR
{
	PhysicsMaterialEditor::PhysicsMaterialEditor()
		: AssetEditor("Edit Physics Material") {}

	void PhysicsMaterialEditor::Close()
	{
		AssetImporter::Serialize(AssetManager::GetMetadata(mAsset->Handle), mAsset);
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

		float textureWidth = (float)mAsset->GetWidth();
		float textureHeight = (float)mAsset->GetHeight();
		float imageSize = ImGui::GetWindowWidth() - 40;
		imageSize = glm::min(imageSize, 500.0f);

		ImGui::SetCursorPosX(20);

		UI::BeginPropertyGrid();		
		UI::Property("Width", textureWidth, 0.1f, 0.0f, 0.0f, true);
		UI::Property("Height", textureHeight, 0.1f, 0.0f, 0.0f, true);
		UI::EndPropertyGrid();
	}

	AudioFileViewer::AudioFileViewer()
		: AssetEditor("Audio File")
	{
		SetMinSize(340, 340);
		SetMaxSize(500, 500);
	}

	void AudioFileViewer::SetAsset(const Ref<Asset>& asset)
	{
		mAsset = (Ref<AudioFile>)asset;
		AssetMetadata& metadata = AssetManager::GetMetadata(mAsset.As<AudioFile>()->Handle);
		SetTitle(metadata.FilePath.stem().string());
	}

	void AudioFileViewer::Close()
	{
		mAsset = nullptr;
	}

	void AudioFileViewer::Render()
	{
		if (!mAsset)
		{
			SetOpen(false);
			return;
		}

		std::chrono::duration<double> seconds{ mAsset->Duration };

		auto duration = Utils::DurationToString(seconds);
		auto samplingRate = std::to_string(mAsset->SamplingRate) + " Hz";
		auto bitDepth = std::to_string(mAsset->BitDepth) + "-bit";
		auto channels = AudioFileUtils::ChannelsToLayoutString(mAsset->NumChannels);
		auto fileSize = Utils::BytesToString(mAsset->FileSize);
		auto filePath = AssetManager::GetMetadata(mAsset->Handle).FilePath.string();
		auto localBounds = ImGui::GetContentRegionAvail();

		localBounds.y -= 14.0f; // making sure to not overlap resize corner

		// Hacky way to hide Property widget background, while making overall text contrast more pleasant
		auto& colors = ImGui::GetStyle().Colors;

		ImGui::PushStyleColor(ImGuiCol_ChildBg, colors[ImGuiCol_FrameBg]);
		ImGui::BeginChild("AudioFileProps", localBounds, false);

		ImGui::Spacing();

		UI::BeginPropertyGrid();

		UI::Property("Duration", duration.c_str());
		UI::Property("Sampling rate", samplingRate.c_str());
		UI::Property("Bit depth", bitDepth.c_str());
		UI::Property("Channels", channels.c_str());
		UI::Property("File size", fileSize.c_str());
		UI::Property("File path", filePath.c_str());

		UI::EndPropertyGrid();

		ImGui::EndChild();
		ImGui::PopStyleColor(); // ImGuiCol_ChildBg
	}

	SoundConfigEditor::SoundConfigEditor()
		: AssetEditor("Sound Configuration")
	{
		SetMinSize(340, 340);
		SetMaxSize(700, 700);
	}

	void SoundConfigEditor::SetAsset(const Ref<Asset>& asset)
	{
		mAsset = (Ref<Audio::SoundConfig>)asset;
		AssetMetadata& metadata = AssetManager::GetMetadata(mAsset.As<Audio::SoundConfig>()->Handle);
		SetTitle(metadata.FilePath.stem().string());
	}

	void SoundConfigEditor::Close()
	{
		AssetImporter::Serialize(AssetManager::GetMetadata(mAsset->Handle), mAsset);
		mAsset = nullptr;
	}

	void SoundConfigEditor::Render()
	{
		if (!mAsset)
		{
			SetOpen(false);
			return;
		}

		auto localBounds = ImGui::GetContentRegionAvail();
		localBounds.y -= 14.0f; // making sure to not overlap resize corner

		const ImVec4 colourDark{ 0.08f,0.08f,0.08f,1.0f };
		const ImVec4 colourDarkest{ 0.04f,0.04f,0.04f,1.0f };
		const ImVec4 colourFrameBG{ 0.14f, 0.14f, 0.14f, 1.0f };

		// Hacky way to hide Property widget background, while making overall text contrast more pleasant
		auto& colors = ImGui::GetStyle().Colors;

		ImGui::PushStyleColor(ImGuiCol_ChildBg, colors[ImGuiCol_FrameBg]);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, colourFrameBG);
		ImGui::PushStyleColor(ImGuiCol_Border, colourDarkest);

		ImGui::BeginChild("SoundConfigPanel", localBounds, false);

		ImGui::Spacing();
		{
			// PropertyGrid consists out of 2 columns, so need to move cursor accordingly
			auto propertyGridSpacing = []
				{
					ImGui::Spacing();
					ImGui::NextColumn();
					ImGui::NextColumn();
				};

			auto singleColumnSeparator = []
				{
					ImDrawList* draw_list = ImGui::GetWindowDrawList();
					ImVec2 p = ImGui::GetCursorScreenPos();
					draw_list->AddLine(ImVec2(p.x - 9999, p.y), ImVec2(p.x + 9999, p.y), ImGui::GetColorU32(ImGuiCol_Border));
				};

			// Making separators a little bit less bright to "separate" them visually from the text
			auto& colors = ImGui::GetStyle().Colors;
			auto oldSCol = colors[ImGuiCol_Separator];
			const float brM = 0.6f;
			colors[ImGuiCol_Separator] = colourDark;

			//=======================================================

			auto& soundConfig = *mAsset;

			// Adding space after header
			ImGui::Spacing();

			//--- Sound Assets and Looping
			//----------------------------
			UI::PushID();
			UI::BeginPropertyGrid();

			// Need to wrap this first Property Grid into another ID,
			// otherwise there's a conflict with the next Property Grid.
			bool bWasEmpty = soundConfig.FileAsset == nullptr;
			if (UI::PropertyAssetReference("Sound", soundConfig.FileAsset))
			{
				if (bWasEmpty)
				{
					soundConfig.FileAsset.Create();
				}
			}

			propertyGridSpacing();
			propertyGridSpacing();

			UI::Property("Volume Multiplier", soundConfig.VolumeMultiplier, 0.01f, 0.0f, 1.0f); //TODO: switch to dBs in the future ?
			UI::Property("Pitch Multiplier", soundConfig.PitchMultiplier, 0.01f, 0.0f, 24.0f); // max pitch 24 is just an arbitrary number here
			
			propertyGridSpacing();
			propertyGridSpacing();
			
			UI::Property("Looping", soundConfig.Looping);

			singleColumnSeparator();
			propertyGridSpacing();
			propertyGridSpacing();

			// trying to make a more usable logarithmic scale for the slider value,
			// so that 0.5 represents 1.2 kHz, or 0.0625 of frequency range normalized
			auto logFrequencyToNormScale = [](float frequencyN)
				{
					return (log2(frequencyN) + 8.0f) / 8.0f;
				};

			auto sliderScaleToLogFreq = [](float sliderValue)
				{
					return pow(2.0f, 8.0f * sliderValue - 8.0f);
				};

			float lpfFreq = 1.0f - soundConfig.LPFilterValue;
			if (UI::Property("Low-Pass Filter", lpfFreq, 0.0f, 0.0f, 1.0f))
			{
				lpfFreq = std::clamp(lpfFreq, 0.0f, 1.0f);
				soundConfig.LPFilterValue = 1.0f - lpfFreq;
			}

			float hpfFreq = soundConfig.HPFilterValue;
			if (UI::Property("High-Pass Filter", hpfFreq, 0.0f, 0.0f, 1.0f))
			{
				hpfFreq = std::clamp(hpfFreq, 0.0f, 1.0f);
				soundConfig.HPFilterValue = hpfFreq;
			}

			singleColumnSeparator();
			propertyGridSpacing();
			propertyGridSpacing();

			UI::Property("Master Reverb send", soundConfig.MasterReverbSend, 0.01f, 0.0f, 1.0f);
			UI::EndPropertyGrid();

			UI::PopID();
			ImGui::Spacing();
			ImGui::Spacing();

			auto contentRegionAvailable = ImGui::GetContentRegionAvail();
			
			ImGui::PopStyleColor(3); // ImGuiCol_ChildBg, ImGuiCol_FrameBg, ImGuiCol_Border

			//--- Enable Spatialization
			//-------------------------
			ImGui::PushStyleColor(ImGuiCol_Header, colourFrameBG);
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, { 0.16f, 0.16f, 0.16f, 1.0f });

			ImGui::Spacing();
			ImGui::Spacing();

			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_CollapsingHeader
				| ImGuiTreeNodeFlags_AllowItemOverlap
				| ImGuiTreeNodeFlags_DefaultOpen
				| ImGuiTreeNodeFlags_SpanAvailWidth;

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 4.0f, 6.0f });

			bool spatializationOpen = ImGui::CollapsingHeader("Spatialization", flags);
			
			ImGui::PopStyleVar();
			ImGui::SameLine(contentRegionAvailable.x - (ImGui::GetFrameHeight() + GImGui->Style.FramePadding.y * 2.0f));
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0f);
			ImGui::Checkbox("##enabled", &soundConfig.SpatializationEnabled);
			ImGui::PopStyleColor(2);

			ImGui::PushStyleColor(ImGuiCol_ChildBg, colors[ImGuiCol_FrameBg]);
			ImGui::PushStyleColor(ImGuiCol_FrameBg, colourFrameBG);
			ImGui::PushStyleColor(ImGuiCol_Border, colourDarkest);
			ImGui::Indent(12.0f);

			//--- Spatialization Settings
			//---------------------------
			if (spatializationOpen)
			{
				ImGui::Spacing();

				using AttModel = Audio::AttenuationModel;

				auto& spatialConfig = soundConfig.Spatialization;
				auto getTextForModel = [&](AttModel model)
					{
						switch (model)
						{
						case AttModel::None:             return "None";
						case AttModel::Inverse:          return "Inverse";
						case AttModel::Linear:           return "Linear";
						case AttModel::Exponential:      return "Exponential";
						default:					     return "None";
						}
					};

				const auto& attenModStr = std::vector<std::string>{ getTextForModel(AttModel::None),
																	getTextForModel(AttModel::Inverse),
																	getTextForModel(AttModel::Linear),
																	getTextForModel(AttModel::Exponential) };
				
				UI::BeginPropertyGrid();
				
				int32_t selectedModel = static_cast<int32_t>(spatialConfig.AttenuationMod);
				if (UI::PropertyDropdown("Attenuaion Model", attenModStr, (int32_t)attenModStr.size(), &selectedModel))
				{
					spatialConfig.AttenuationMod = static_cast<AttModel>(selectedModel);
				}

				singleColumnSeparator();
				propertyGridSpacing();
				propertyGridSpacing();
				
				UI::Property("Min Gain", spatialConfig.MinGain, 0.01f, 0.0f, 1.0f);
				UI::Property("Max Gain", spatialConfig.MaxGain, 0.01f, 0.0f, 1.0f);
				UI::Property("Min Distance", spatialConfig.MinDistance, 1.00f, 0.0f, FLT_MAX);
				UI::Property("Max Distance", spatialConfig.MaxDistance, 1.00f, 0.0f, FLT_MAX);
				
				singleColumnSeparator();
				propertyGridSpacing();
				propertyGridSpacing();
				
				float inAngle = glm::degrees(spatialConfig.ConeInnerAngleInRadians);
				float outAngle = glm::degrees(spatialConfig.ConeOuterAngleInRadians);
				float outGain = spatialConfig.ConeOuterGain;
				
				// Manually clamp here because UI::Property doesn't take flags to pass in ImGuiSliderFlags_ClampOnInput
				if (UI::Property("Cone Inner Angle", inAngle, 1.0f, 0.0f, 360.0f))
				{
					if (inAngle > 360.0f) 
					{
						inAngle = 360.0f;
					}
					spatialConfig.ConeInnerAngleInRadians = glm::radians(inAngle);
				}
				if (UI::Property("Cone Outer Angle", outAngle, 1.0f, 0.0f, 360.0f))
				{
					if (outAngle > 360.0f) 
					{
						outAngle = 360.0f;
					}
					spatialConfig.ConeOuterAngleInRadians = glm::radians(outAngle);
				}
				if (UI::Property("Cone Outer Gain", outGain, 0.01f, 0.0f, 1.0f))
				{
					if (outGain > 1.0f) 
					{
						outGain = 1.0f;
					}
					spatialConfig.ConeOuterGain = outGain;
				}

				singleColumnSeparator();
				propertyGridSpacing();
				propertyGridSpacing();
				
				UI::Property("Doppler Factor", spatialConfig.DopplerFactor, 0.01f, 0.0f, 1.0f);
				
				propertyGridSpacing();
				propertyGridSpacing();
				
				UI::EndPropertyGrid();
			}

			colors[ImGuiCol_Separator] = oldSCol;
			ImGui::Unindent();
		}

		ImGui::EndChild(); // SoundConfigPanel
		ImGui::PopStyleColor(3); // ImGuiCol_ChildBg, ImGuiCol_FrameBg, ImGuiCol_Border
	}
}