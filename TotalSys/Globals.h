#pragma once

#include "ProcessInfoEx.h"
#include "ThreadInfoEx.h"
#include <ProcessManager.h>

struct TotalSysSettings;

struct Globals abstract final {
	static void SetMonoFont(ImFont* font);
	static void SetVarFont(ImFont* font);
	static ImFont* MonoFont();
	static ImFont* VarFont();
	static bool IsDarkMode();
	static void SetDarkMode(bool dark);

	static TotalSysSettings& Settings();

	inline static ImFont* s_pMonoFont;
	inline static ImFont* s_pVarFont;
	inline static bool s_Dark{ true };
};

