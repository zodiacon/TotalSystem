#pragma once

#include "ProcessColor.h"

struct TotalSysSettings {
	TotalSysSettings();

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

	uint32_t NewObjectsTime{ 2000 };
	uint32_t OldObjectsTime{ 2000 };

	std::vector<ProcessColor>& GetProcessColors();

private:
	std::vector<ProcessColor> m_ProcessColors[2];

};

