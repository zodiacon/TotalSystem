#pragma once

#include "imgui.h"
#include <string>

namespace ImGui {
	bool ButtonEnabled(const char* label, bool enabled, const ImVec2& size = ImVec2(0, 0));
	bool ButtonColoredEnabled(const char* label, const ImVec4& color, bool enabled, const ImVec2& size = ImVec2(0, 0));
	bool InputTextReadonly(const char* label, std::string& text, float width = 0.0f, ImGuiInputTextFlags flags = ImGuiInputTextFlags_ReadOnly);
};

