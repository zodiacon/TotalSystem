#include "pch.h"
#include "TotalSysSettings.h"
#include <StandardColors.h>
#include "FormatHelper.h"
#include "Globals.h"

using namespace ImGui;

TotalSysSettings::TotalSysSettings() {
	auto& black = Colors::Black;
	auto& white = Colors::White;

	m_ProcessColors[0] = {
		ProcessColor(FormatHelper::ColorWithAlpha(Colors::DarkGreen, .9f), white),
		ProcessColor(FormatHelper::ColorWithAlpha(Colors::DarkRed, .9f), white),
		ProcessColor(FormatHelper::ColorWithAlpha(Colors::DarkOrange, .6f), white),
		ProcessColor(FormatHelper::ColorWithAlpha(Colors::DarkCyan, .6f), white),
		ProcessColor(FormatHelper::ColorWithAlpha(Colors::DarkSalmon, .6f), white),
		ProcessColor(FormatHelper::ColorWithAlpha(Colors::Fuchsia, .6f), white),
		ProcessColor(FormatHelper::ColorWithAlpha(Colors::Purple, .6f), white),
		ProcessColor(FormatHelper::ColorWithAlpha(Colors::Brown, .7f), white, false),
		ProcessColor(FormatHelper::ColorWithAlpha(Colors::DimGray, .9f), white, true),
	};
	m_ProcessColors[1] = {
		ProcessColor(FormatHelper::ColorWithAlpha(Colors::LightGreen, .9f), black),
		ProcessColor(FormatHelper::ColorWithAlpha(Colors::Red, .9f), black),
		ProcessColor(FormatHelper::ColorWithAlpha(Colors::Orange, .6f), black),
		ProcessColor(FormatHelper::ColorWithAlpha(Colors::Cyan, .6f), black),
		ProcessColor(FormatHelper::ColorWithAlpha(Colors::Salmon, .6f), black),
		ProcessColor(FormatHelper::ColorWithAlpha(Colors::Fuchsia, .6f), black),
		ProcessColor(FormatHelper::ColorWithAlpha(Colors::Magenta, .6f), black),
		ProcessColor(FormatHelper::ColorWithAlpha(Colors::Brown, .7f), black, false),
		ProcessColor(FormatHelper::ColorWithAlpha(Colors::Gray, .8f), black, true),
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
