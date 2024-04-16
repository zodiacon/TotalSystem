#include "pch.h"
#include "ProcMgrSettings.h"
#include <StandardColors.h>
#include "FormatHelpers.h"

using namespace ImGui;

ProcMgrSettings::ProcMgrSettings() {
	auto& black = StandardColors::Black;
	auto& white = StandardColors::White;

	ProcessColors = {
		ProcessColor("Created", FormatHelpers::ColorWithAlpha(StandardColors::DarkGreen, .9f), white),
		ProcessColor("Terminated", FormatHelpers::ColorWithAlpha(StandardColors::DarkRed, .9f), white),
		ProcessColor("Managed (.NET)", FormatHelpers::ColorWithAlpha(StandardColors::DarkOrange, .6f), white),
		ProcessColor("Immersive", FormatHelpers::ColorWithAlpha(StandardColors::DarkCyan, .6f), white),
		ProcessColor("Services", FormatHelpers::ColorWithAlpha(StandardColors::DarkSalmon, .6f), white),
		ProcessColor("Protected", FormatHelpers::ColorWithAlpha(StandardColors::Fuchsia, .6f), white),
		ProcessColor("Secure", FormatHelpers::ColorWithAlpha(StandardColors::Purple, .6f), white),
		ProcessColor("In Job", FormatHelpers::ColorWithAlpha(StandardColors::Brown, .6f), white, false),
		ProcessColor("Suspended", FormatHelpers::ColorWithAlpha(StandardColors::DarkGray, .6f), white, false),
	};
}
