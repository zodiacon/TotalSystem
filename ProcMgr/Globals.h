#pragma once

#include "ProcessInfoEx.h"
#include "ThreadInfoEx.h"
#include <ProcessManager.h>

struct ProcMgrSettings;

struct Globals abstract final {
	static auto& ProcessManager() {
		static WinLL::ProcessManager<ProcessInfoEx, ThreadInfoEx> pm;
		return pm;
	}

	static void SetMonoFont(ImFont* font);
	static void SetVarFont(ImFont* font);
	static ImFont* MonoFont();
	static ImFont* VarFont();

	static ProcMgrSettings& Settings();

	inline static ImFont* s_pMonoFont;
	inline static ImFont* s_pVarFont;
};

