#include "pch.h"
#include "TotalSysSettings.h"
#include <StandardColors.h>
#include "FormatHelper.h"
#include "Globals.h"

using namespace ImGui;

TotalSysSettings::TotalSysSettings() {
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
		ProcessColor("In Job", FormatHelper::ColorWithAlpha(StandardColors::Brown, .6f), white, false),
		ProcessColor("Suspended", FormatHelper::ColorWithAlpha(StandardColors::DarkGray, .6f), white, false),
	};
	m_ProcessColors[1] = {
		ProcessColor("Created", FormatHelper::ColorWithAlpha(StandardColors::LightGreen, .9f), black),
		ProcessColor("Terminated", FormatHelper::ColorWithAlpha(StandardColors::Red, .9f), black),
		ProcessColor("Managed (.NET)", FormatHelper::ColorWithAlpha(StandardColors::Orange, .6f), black),
		ProcessColor("Immersive", FormatHelper::ColorWithAlpha(StandardColors::Cyan, .6f), black),
		ProcessColor("Services", FormatHelper::ColorWithAlpha(StandardColors::Salmon, .6f), black),
		ProcessColor("Protected", FormatHelper::ColorWithAlpha(StandardColors::Fuchsia, .6f), black),
		ProcessColor("Secure", FormatHelper::ColorWithAlpha(StandardColors::Magenta, .6f), black),
		ProcessColor("In Job", FormatHelper::ColorWithAlpha(StandardColors::Brown, .6f), black, false),
		ProcessColor("Suspended", FormatHelper::ColorWithAlpha(StandardColors::Gray, .6f), black, false),
	};

}

std::vector<ProcessColor>& TotalSysSettings::GetProcessColors() {
	return m_ProcessColors[Globals::IsDarkMode() ? 0 : 1];
}
