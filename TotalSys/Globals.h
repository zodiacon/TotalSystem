#pragma once

#include "ProcessInfoEx.h"
#include "ThreadInfoEx.h"
#include <ProcessManager.h>

struct TotalSysSettings;

struct Globals abstract final {
	static auto& ProcessManager() {
		static WinLL::ProcessManager<ProcessInfoEx, ThreadInfoEx> pm;
		return pm;
	}

	static void SetMonoFont(ImFont* font);
	static void SetVarFont(ImFont* font);
	static ImFont* MonoFont();
	static ImFont* VarFont();

	static TotalSysSettings& Settings();

	inline static ImFont* s_pMonoFont;
	inline static ImFont* s_pVarFont;
};

