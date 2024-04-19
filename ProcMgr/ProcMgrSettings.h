#pragma once

#include "ProcessColor.h"

struct ProcMgrSettings {
	ProcMgrSettings();

	enum ProcessColorIndex {
		NewObjects,
		DeletedObjects,
		Manageed,
		Immersive,
		Services,
		Protected,
		Secure,
		InJob,
		Suspended,
	};

	std::vector<ProcessColor> ProcessColors;

};

