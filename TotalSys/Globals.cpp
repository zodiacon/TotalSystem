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

TotalSysSettings& Globals::Settings() {
	static TotalSysSettings settings;
	return settings;
}
