#include "pch.h"
#include "TotalSysSettings.h"
#include <StandardColors.h>
#include "FormatHelper.h"
#include "Globals.h"

using namespace ImGui;

TotalSysSettings::TotalSysSettings() {
	InitSettings();

	auto& black = StandardColors::Black;
	auto& white = StandardColors::White;

	m_ProcessColors[0] = {
		ProcessColor("Created", FormatHelper::ColorWithAlpha(StandardColors::DarkGreen, .9f), white),
		ProcessColor("Terminated", FormatHelper::ColorWithAlpha(StandardColors::DarkRed, .9f), white),
		ProcessColor("Managed (.NET)", FormatHelper::ColorWithAlpha(StandardColors::DarkOrange, .6f), white),
		ProcessColor("Immersive", FormatHelper::ColorWithAlpha(StandardColors::DarkCyan, .6f), white),
		ProcessColor("Services", FormatHelper::ColorWithAlpha(StandardColors::DarkSalmon, .6f), white),
		ProcessColor("Protected", FormatHelper::ColorWithAlpha(StandardColors::Fuchsia, .6f), white),
		ProcessColor("Secure", FormatHelper::ColorWithAlpha(StandardColors::Purple, .6f), white),
		ProcessColor("In Job", FormatHelper::ColorWithAlpha(StandardColors::Brown, .7f), white, false),
		ProcessColor("Suspended", FormatHelper::ColorWithAlpha(StandardColors::DimGray, .9f), white, true),
	};
	m_ProcessColors[1] = {
		ProcessColor("Created", FormatHelper::ColorWithAlpha(StandardColors::LightGreen, .9f), black),
		ProcessColor("Terminated", FormatHelper::ColorWithAlpha(StandardColors::Red, .9f), black),
		ProcessColor("Managed (.NET)", FormatHelper::ColorWithAlpha(StandardColors::Orange, .6f), black),
		ProcessColor("Immersive", FormatHelper::ColorWithAlpha(StandardColors::Cyan, .6f), black),
		ProcessColor("Services", FormatHelper::ColorWithAlpha(StandardColors::Salmon, .6f), black),
		ProcessColor("Protected", FormatHelper::ColorWithAlpha(StandardColors::Fuchsia, .6f), black),
		ProcessColor("Secure", FormatHelper::ColorWithAlpha(StandardColors::Magenta, .6f), black),
		ProcessColor("In Job", FormatHelper::ColorWithAlpha(StandardColors::Brown, .7f), black, false),
		ProcessColor("Suspended", FormatHelper::ColorWithAlpha(StandardColors::Gray, .8f), black, true),
	};
}

std::vector<ProcessColor>& TotalSysSettings::GetProcessColors() {
	return m_ProcessColors[DarkMode() ? 0 : 1];
}

ImVec4 TotalSysSettings::GetRelocatedColor() const {
	return GetAddress<ImVec4>(L"RelocatedColor")[DarkMode() ? 0 : 1];
}
