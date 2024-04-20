#include "pch.h"
#include "TotalSysSettings.h"
#include <StandardColors.h>
#include "FormatHelper.h"

using namespace ImGui;

TotalSysSettings::TotalSysSettings() {
	auto& black = StandardColors::Black;
	auto& white = StandardColors::White;

	ProcessColors = {
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
}
