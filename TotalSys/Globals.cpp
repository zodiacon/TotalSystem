#include "pch.h"
#include "Globals.h"
#include "TotalSysSettings.h"

void Globals::SetMonoFont(ImFont* font) {
	s_pMonoFont = font;
}

void Globals::SetVarFont(ImFont* font) {
	s_pVarFont = font;
}

ImFont* Globals::MonoFont() {
	return s_pMonoFont;
}

ImFont* Globals::VarFont() {
	return s_pVarFont;
}

bool Globals::IsDarkMode() {
	return s_Dark;
}

void Globals::SetDarkMode(bool dark) {
	if (s_Dark = dark)
		ImGui::StyleColorsDark();
	else
		ImGui::StyleColorsLight();

}

IconManager& Globals::ImageManager() {
	static IconManager mgr;
	return mgr;
}

TotalSysSettings& Globals::Settings() {
	static TotalSysSettings settings;
	return settings;
}
