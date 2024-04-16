#pragma once

#include "ProcessInfoEx.h"
#include <ProcessManager.h>

struct ProcMgrSettings;

struct Globals abstract final {
	static auto& ProcessManager() {
		static WinLL::ProcessManager<ProcessInfoEx> pm;
		return pm;
	}

	static ProcMgrSettings& Settings();
};

