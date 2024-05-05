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
	return Settings().DarkMode();
}

void Globals::SetDarkMode(bool dark) {
	Settings().DarkMode(dark);
	if(dark)
		ImGui::StyleColorsDark();
	else
		ImGui::StyleColorsLight();
	Settings().ThemeAsSystem(false);

}

IconManager& Globals::ImageManager() {
	static IconManager mgr;
	return mgr;
}

TotalSysSettings& Globals::Settings() {
	static TotalSysSettings settings;
	return settings;
}

MainWindow& Globals::RootWindow() {
	return *s_MainWindow;
}

void Globals::SetMainWindow(MainWindow* win) {
	assert(s_MainWindow == nullptr);
	s_MainWindow = win;
}

void Globals::SetAsSystem(bool dark) {
	Settings().DarkMode(dark);
	Settings().ThemeAsSystem(true);
}
