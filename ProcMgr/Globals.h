#pragma once

#include "ProcessInfoEx.h"
#include <ProcessManager.h>

struct Globals abstract final {
	static auto& ProcessManager() {
		static WinLL::ProcessManager<ProcessInfoEx> pm;
		return pm;
	}
};

