#include "nrpch.h"
#include "SceneAudio.h"

#include "NotRed/Audio/AudioEngine.h"
#include "NotRed/Audio/DSP/Reverb/Reverb.h"

#include "NotRed/ImGui/ImGui.h"

namespace NR::Audio
{
	void SceneAudio::ImGuiRender()
	{
		ImGui::Begin("Scene Audio");

		if (UI::BeginTreeNode("Master Reverb"))
		{
			if (auto* masterReverb = Audio::AudioEngine::Get().GetMasterReverb())
			{
				UI::BeginPropertyGrid();

				float preDelay = masterReverb->GetParameter(DSP::EReverbParameters::PreDelay);
				float roomSize = masterReverb->GetParameter(DSP::EReverbParameters::RoomSize) / 0.98f; // Reverb algorithm requirment
				float damping = masterReverb->GetParameter(DSP::EReverbParameters::Damp);
				float width = masterReverb->GetParameter(DSP::EReverbParameters::Width);
				
				if (UI::Property(masterReverb->GetParameterName(DSP::EReverbParameters::PreDelay), preDelay, 1.0f, 0.0f, 1000.0f))
				{
					preDelay = std::min(preDelay, 1000.0f);
					masterReverb->SetParameter(DSP::EReverbParameters::PreDelay, preDelay);
				}

				if (UI::Property(masterReverb->GetParameterName(DSP::EReverbParameters::RoomSize), roomSize, 0.0f, 0.0f, 1.0f))
				{
					masterReverb->SetParameter(DSP::EReverbParameters::RoomSize, roomSize * 0.98f);
				}

				if (UI::Property(masterReverb->GetParameterName(DSP::EReverbParameters::Damp), damping, 0.0f, 0.0f, 2.0f))
				{
					damping = std::min(damping, 2.0f);  // Reverb algorithm requirment
					masterReverb->SetParameter(DSP::EReverbParameters::Damp, damping);
				}

				if (UI::Property(masterReverb->GetParameterName(DSP::EReverbParameters::Width), width, 0.0f, 0.0f, 1.0f))
				{
					masterReverb->SetParameter(DSP::EReverbParameters::Width, width);
				}

				UI::EndPropertyGrid();
			}

			UI::EndTreeNode();
		}

		ImGui::End();
	}
}