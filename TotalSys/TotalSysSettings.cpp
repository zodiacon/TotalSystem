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
		ProcessColor(FormatHelper::ColorWithAlpha(StandardColors::DarkGreen, .9f), white),
		ProcessColor(FormatHelper::ColorWithAlpha(StandardColors::DarkRed, .9f), white),
		ProcessColor(FormatHelper::ColorWithAlpha(StandardColors::DarkOrange, .6f), white),
		ProcessColor(FormatHelper::ColorWithAlpha(StandardColors::DarkCyan, .6f), white),
		ProcessColor(FormatHelper::ColorWithAlpha(StandardColors::DarkSalmon, .6f), white),
		ProcessColor(FormatHelper::ColorWithAlpha(StandardColors::Fuchsia, .6f), white),
		ProcessColor(FormatHelper::ColorWithAlpha(StandardColors::Purple, .6f), white),
		ProcessColor(FormatHelper::ColorWithAlpha(StandardColors::Brown, .7f), white, false),
		ProcessColor(FormatHelper::ColorWithAlpha(StandardColors::DimGray, .9f), white, true),
	};
	m_ProcessColors[1] = {
		ProcessColor(FormatHelper::ColorWithAlpha(StandardColors::LightGreen, .9f), black),
		ProcessColor(FormatHelper::ColorWithAlpha(StandardColors::Red, .9f), black),
		ProcessColor(FormatHelper::ColorWithAlpha(StandardColors::Orange, .6f), black),
		ProcessColor(FormatHelper::ColorWithAlpha(StandardColors::Cyan, .6f), black),
		ProcessColor(FormatHelper::ColorWithAlpha(StandardColors::Salmon, .6f), black),
		ProcessColor(FormatHelper::ColorWithAlpha(StandardColors::Fuchsia, .6f), black),
		ProcessColor(FormatHelper::ColorWithAlpha(StandardColors::Magenta, .6f), black),
		ProcessColor(FormatHelper::ColorWithAlpha(StandardColors::Brown, .7f), black, false),
		ProcessColor(FormatHelper::ColorWithAlpha(StandardColors::Gray, .8f), black, true),
	};

	InitSettings();

}

ProcessColor* TotalSysSettings::GetProcessColors() {
	return GetAddress<ProcessColor>(DarkMode() ? L"ProcessColorsDark" : L"ProcessColorsLight");
}

ImVec4 TotalSysSettings::GetRelocatedColor() const {
	return GetAddress<ImVec4>(L"RelocatedColor")[DarkMode() ? 0 : 1];
}

const char* TotalSysSettings::GetColorIndexName(int n) {
	const char* names[] = {
		"Created", 
		"Terminated", 
		"Managed",
		"Immersive",
		"Services", 
		"Protected",
		"Secure",
		"In Job",
		"Suspended",
	};
	return names[n];
}
