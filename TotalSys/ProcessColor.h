#pragma once

#include "imgui.h"

struct ProcessColor {
	ProcessColor() = default;
	ProcessColor(const ImVec4& defaultColor, const ImVec4& defaultTextColor, bool enabled = true);

	ImVec4 DefaultColor;
	ImVec4 Color;
	ImVec4 DefaultTextColor;
	ImVec4 TextColor;
	bool Enabled;
};

