#include "pch.h"
#include "ProcMgrSettings.h"
#include <StandardColors.h>
#include "FormatHelpers.h"

using namespace ImGui;

ProcMgrSettings::ProcMgrSettings() {
	auto& black = StandardColors::Black;
	auto& white = StandardColors::White;

	ProcessColors = {
		ProcessColor("New Objects", FormatHelpers::ColorWithAlpha(StandardColors::DarkGreen, .9f), white),
		ProcessColor("Deleted Objects", FormatHelpers::ColorWithAlpha(StandardColors::DarkRed, .9f), white),
		ProcessColor("Managed (.NET)", FormatHelpers::ColorWithAlpha(StandardColors::DarkOrange, .7f), white),
		ProcessColor("Immersive", FormatHelpers::ColorWithAlpha(StandardColors::DarkCyan, .7f), white),
		ProcessColor("Services", FormatHelpers::ColorWithAlpha(StandardColors::DarkSalmon, .7f), white),
		ProcessColor("Protected", FormatHelpers::ColorWithAlpha(StandardColors::Fuchsia, .7f), white),
		ProcessColor("Secure", FormatHelpers::ColorWithAlpha(StandardColors::Purple, .7f), white),
		ProcessColor("In Job", FormatHelpers::ColorWithAlpha(StandardColors::Brown, .7f), white, false),
		ProcessColor("Suspended", FormatHelpers::ColorWithAlpha(StandardColors::DarkGray, .7f), white, false),
	};
}
