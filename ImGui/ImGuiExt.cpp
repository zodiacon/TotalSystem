#include "ImGuiExt.h"

namespace ImGui {
	bool ButtonEnabled(const char* label, bool enabled, const ImVec2& size) {
		if (!enabled) {
			PushStyleColor(ImGuiCol_Button, GetStyle().Colors[ImGuiCol_TextDisabled]);
			PushStyleColor(ImGuiCol_ButtonHovered, GetStyle().Colors[ImGuiCol_Button]);
		}
		auto clicked = Button(label, size);
		if (!enabled)
			PopStyleColor(2);

		return enabled && clicked;
	}

	bool ButtonColoredEnabled(const char* label, const ImVec4& color, bool enabled, const ImVec2& size) {
		return false;
	}

	bool InputTextReadonly(const char* label, std::string& text, float width, ImGuiInputTextFlags flags) {
		if (width == 0) {
			width = CalcTextSize(text.c_str()).x + 20;
			auto size = CalcTextSize(label).x + 10;
			if (size + width < GetWindowWidth())
				SetNextItemWidth(width);
		}
		else if (width > 0) {
			SetNextItemWidth(width);
		}
		return InputText(label, text.data(), text.length(), flags);
	}
}
