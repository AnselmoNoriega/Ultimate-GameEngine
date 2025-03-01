#pragma once

#include "EditorPanel.h"

#include "EditorConsole/ConsoleMessage.h"
#include "NotRed/Renderer/Texture.h"

namespace NR
{
	class EditorConsolePanel : public EditorPanel
	{
	public:
		EditorConsolePanel();
		~EditorConsolePanel();

		void ImGuiRender(bool& isOpen) override;
		void ScenePlay();

	private:
		void RenderMenu();
		void RenderConsole();

	private:
		static void PushMessage(const ConsoleMessage& message);

	private:
		Ref<Texture2D> mInfoButtonTex, mWarningButtonTex, mErrorButtonTex;

		bool mShouldClearOnPlay = false;
		bool mCollapseMessages = false;
		bool mNewMessageAdded = false;

		static constexpr uint32_t sMessageBufferCapacity = 500;
		std::array<ConsoleMessage, sMessageBufferCapacity> mMessageBuffer;

		uint32_t mMessageBufferBegin = 0;
		int32_t mMessageFilters = (int16_t)ConsoleMessage::Category::Info | (int16_t)ConsoleMessage::Category::Warning | (int16_t)ConsoleMessage::Category::Error;

		ConsoleMessage* mSelectedMessage = nullptr;

		bool mDisplayMessageInspector = false;
		bool mIsMessageInspectorHovered = false;

	private:
		friend class EditorConsoleSink;
	};
}