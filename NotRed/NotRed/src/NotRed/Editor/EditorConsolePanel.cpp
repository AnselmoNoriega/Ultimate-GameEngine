#include "nrpch.h"
#include "EditorConsolePanel.h"

#include "NotRed/ImGui/ImGui.h"

#include "imgui/imgui_internal.h"

namespace NR
{
	static EditorConsolePanel* sInstance = nullptr;

	static ImVec4 sInfoButtonTint = ImVec4(0.0f, 0.431372549f, 1.0f, 1.0f);
	static ImVec4 sWarningButtonTint = ImVec4(1.0f, 0.890196078f, 0.0588235294f, 1.0f);
	static ImVec4 sErrorButtonTint = ImVec4(1.0f, 0.309803922f, 0.309803922f, 1.0f);
	static ImVec4 sNoTint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

	EditorConsolePanel::EditorConsolePanel()
	{
		NR_CORE_ASSERT(sInstance == nullptr);
		sInstance = this;

		mInfoButtonTex = Texture2D::Create("Resources/Editor/InfoButton.png");
		mWarningButtonTex = Texture2D::Create("Resources/Editor/WarningButton.png");
		mErrorButtonTex = Texture2D::Create("Resources/Editor/ErrorButton.png");
	}

	EditorConsolePanel::~EditorConsolePanel()
	{
		sInstance = nullptr;
	}

	void EditorConsolePanel::ImGuiRender(bool* show)
	{
		if (!(*show))
		{
			return;
		}

		ImGui::Begin("Log", show);
		RenderMenu();
		ImGui::Separator();
		RenderConsole();
		ImGui::End();
	}

	void EditorConsolePanel::ScenePlay()
	{
		if (mShouldClearOnPlay)
		{
			mMessageBufferBegin = 0;
		}
	}

	void EditorConsolePanel::RenderMenu()
	{
		ImVec4 infoButtonTint = (mMessageFilters & (int16_t)ConsoleMessage::Category::Info) ? sInfoButtonTint : sNoTint;
		ImVec4 warningButtonTint = (mMessageFilters & (int16_t)ConsoleMessage::Category::Info) ? sWarningButtonTint : sNoTint;
		ImVec4 errorButtonTint = (mMessageFilters & (int16_t)ConsoleMessage::Category::Info) ? sErrorButtonTint : sNoTint;

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 5));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(5, 3));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));

		if (ImGui::Button("Clear"))
		{
			mMessageBufferBegin = 0;
		}

		ImGui::SameLine(0.0f, 5.0f);
		ImGui::TextUnformatted("Clear On Play:");
		ImGui::SameLine();
		ImGui::Checkbox("##ClearOnPlay", &mShouldClearOnPlay);

		ImGui::SameLine(0.0f, 5.0f);
		ImGui::TextUnformatted("Collapse:");
		ImGui::SameLine();
		ImGui::Checkbox("##CollapseMessages", &mCollapseMessages);

		constexpr float buttonOffset = 39;
		constexpr float rightSideOffset = 15;

		ImGui::SameLine(ImGui::GetWindowWidth() - (buttonOffset * 3) - rightSideOffset);
		if (UI::ImageButton(mInfoButtonTex, ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0, 0, 0, 0), infoButtonTint))
		{
			mMessageFilters ^= (int16_t)ConsoleMessage::Category::Info;
		}

		ImGui::SameLine(ImGui::GetWindowWidth() - (buttonOffset * 2) - rightSideOffset);
		if (UI::ImageButton(mWarningButtonTex, ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0, 0, 0, 0), warningButtonTint))
		{
			mMessageFilters ^= (int16_t)ConsoleMessage::Category::Warning;
		}

		ImGui::SameLine(ImGui::GetWindowWidth() - (buttonOffset * 1) - rightSideOffset);
		if (UI::ImageButton(mErrorButtonTex, ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0, 0, 0, 0), errorButtonTint))
		{
			mMessageFilters ^= (int16_t)ConsoleMessage::Category::Error;
		}

		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(2);
	}

	void EditorConsolePanel::RenderConsole()
	{
		ImGui::BeginChild("LogMessages");

		if (mMessageBufferBegin == 0)
		{
			mDisplayMessageInspector = false;
			mSelectedMessage = nullptr;
		}

		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !mIsMessageInspectorHovered)
		{
			mDisplayMessageInspector = false;
			mSelectedMessage = nullptr;
		}

		for (uint32_t i = 0; i < mMessageBufferBegin; ++i)
		{
			const auto& msg = mMessageBuffer[i];
			if (mMessageFilters & (int16_t)msg.GetCategory())
			{
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 5));

				if (i % 2 == 0)
				{
					ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(ImVec4(0.20f, 0.20f, 0.20f, 1.0f)));
				}

				ImGui::BeginChild(i + 1, ImVec2(0, ImGui::GetFontSize() * 1.75F), false, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysUseWindowPadding);

				if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				{
					mSelectedMessage = &mMessageBuffer[i];
					mDisplayMessageInspector = true;
				}

				std::string messageText = msg.GetMessage();

				if (ImGui::BeginPopupContextWindow())
				{
					if (ImGui::MenuItem("Copy"))
					{
						ImGui::SetClipboardText(messageText.c_str());
					}

					ImGui::EndPopup();
				}

				if (msg.GetCategory() == ConsoleMessage::Category::Info)
				{
					UI::Image(mInfoButtonTex, ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1), sInfoButtonTint);
				}
				else if (msg.GetCategory() == ConsoleMessage::Category::Warning)
				{
					UI::Image(mWarningButtonTex, ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1), sWarningButtonTint);
				}
				else if (msg.GetCategory() == ConsoleMessage::Category::Error)
				{
					UI::Image(mErrorButtonTex, ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1), sErrorButtonTint);
				}

				ImGui::SameLine();

				if (messageText.length() > 200)
				{
					size_t spacePos = messageText.find_first_of(' ', 200);
					if (spacePos != std::string::npos)
					{
						messageText.replace(spacePos, messageText.length() - 1, "...");
					}
				}

				ImGui::TextUnformatted(messageText.c_str());

				if (mCollapseMessages && msg.GetCount() > 1)
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 30);
					ImGui::Text("%d", msg.GetCount());
				}

				ImGui::EndChild();

				if (i % 2 == 0)
				{
					ImGui::PopStyleColor();
				}

				ImGui::PopStyleVar();
			}
		}

		if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() && !mDisplayMessageInspector)
		{
			ImGui::SetScrollHereY(1.0f);
		}

		if (mDisplayMessageInspector && mSelectedMessage != nullptr)
		{
			// TOOD(Peter): Ensure that this panel is always docked to the bottom of the Log panel
			static bool sIsMessageInspectorDocked = false;
			if (sIsMessageInspectorDocked)
			{
				ImGuiWindowClass window_class;
				window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;
				ImGui::SetNextWindowClass(&window_class);
			}

			ImGui::Begin("##MessageInspector");

			sIsMessageInspectorDocked = ImGui::IsWindowDocked();
			mIsMessageInspectorHovered = ImGui::IsWindowHovered();

			ImGui::PushTextWrapPos();
			const auto& msg = mSelectedMessage->GetMessage();
			ImGui::TextUnformatted(msg.c_str());
			ImGui::PopTextWrapPos();

			ImGui::End();
		}
		else
		{
			mIsMessageInspectorHovered = false;
		}

		ImGui::EndChild();

	}

	void EditorConsolePanel::PushMessage(const ConsoleMessage& message)
	{
		if (sInstance == nullptr)
		{
			return;
		}

		if (message.GetCategory() == ConsoleMessage::Category::None)
		{
			return;
		}

		if (sInstance->mCollapseMessages)
		{
			for (auto& other : sInstance->mMessageBuffer)
			{
				if (message.GetMessageID() == other.GetMessageID())
				{
					++other.mCount;
					return;
				}
			}
		}

		sInstance->mMessageBuffer[sInstance->mMessageBufferBegin++] = message;

		if (sInstance->mMessageBufferBegin == sMessageBufferCapacity)
		{
			sInstance->mMessageBufferBegin = 0;
		}
	}
}