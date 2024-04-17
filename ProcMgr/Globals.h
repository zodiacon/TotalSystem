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

	static ProcMgrSettings& Settings();
};

