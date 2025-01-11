#pragma once
#include <imgui.h>

namespace Colors
{
	static inline float Convert_sRGB_FromLinear(float theLinearValue);
	static inline float Convert_sRGB_ToLinear(float thesRGBValue);
	ImVec4 ConvertFromSRGB(ImVec4 colour);
	ImVec4 ConvertToSRGB(ImVec4 colour);

	const ImVec4 accent{ 0.9254901960784314f, 0.6196078431372549f, 0.1411764705882353f, 1.0f };
	const ImVec4 highlight{ 0.1529411764705882f, 0.7254901960784314f, 0.9490196078431373f, 1.0f };
	const ImVec4 compliment{ 0.3058823529411765f, 0.592156862745098f, 0.6509803921568627f, 1.0f };

	const ImVec4 BG{ 0.1490196078431373f, 0.1490196078431373f, 0.1490196078431373f, 1.0f };					// 38
	const ImVec4 BGDarkSeparators{ 0.1019607843137255f, 0.1019607843137255f, 0.1019607843137255f, 1.0f };	// 26
	const ImVec4 BGTitlebar{ 0.0823529411764706f, 0.0823529411764706f, 0.0823529411764706f, 1.0f };			// 21
	const ImVec4 BGPropertyField{ 0.0588235294117647f, 0.0588235294117647f, 0.0588235294117647f, 1.0f };		// 15

	constexpr ImU32 text = IM_COL32(192, 192, 192, 255);
	constexpr ImU32 textDarker = IM_COL32(128, 128, 128, 255);

	const ImVec4 muted = { 0.3019607843137255f, 0.3019607843137255f, 0.3019607843137255f, 1.0f };							// 77

	const ImVec4 groupHeader = { 0.1843137254901961f, 0.1843137254901961f, 0.1843137254901961f, 1.0f };						// 47

	const ImVec4 selected{ 0.7490196078431373f, 0.6941176470588235f, 0.607843137254902f, 1.0f };
	const ImVec4 hovered{ 0.3490196078431373f, 0.3254901960784314f, 0.2823529411764706f, 1.0f };
	const ImVec4 clicked{ 0.7490196078431373f, 0.6941176470588235f, 0.607843137254902f, 1.0f };
	const ImVec4 selectionMuted{ 0.2313725490196078f, 0.2235294117647059f, 0.1764705882352941f, 1.0f };

	namespace Theme
	{
		constexpr auto accent = IM_COL32(236, 158, 36, 255);
		constexpr auto highlight = IM_COL32(39, 185, 242, 255);
		constexpr auto niceBlue = IM_COL32(83, 232, 254, 255);
		constexpr auto compliment = IM_COL32(78, 151, 166, 255);
		constexpr auto background = IM_COL32(38, 38, 38, 255);
		constexpr auto backgroundDark = IM_COL32(26, 26, 26, 255);
		constexpr auto titlebar = IM_COL32(21, 21, 21, 255);
		constexpr auto propertyField = IM_COL32(15, 15, 15, 255);
		constexpr auto text = IM_COL32(192, 192, 192, 255);
		constexpr auto textBrighter = IM_COL32(210, 210, 210, 255);
		constexpr auto textDarker = IM_COL32(128, 128, 128, 255);
		constexpr auto muted = IM_COL32(77, 77, 77, 255);
		constexpr auto groupHeader = IM_COL32(47, 47, 47, 255);
		constexpr auto selection = IM_COL32(191, 177, 155, 255);
		constexpr auto selectionMuted = IM_COL32(59, 57, 45, 255);

		constexpr auto backgroundPopup = IM_COL32(68, 70, 77, 255);
	}
}