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

	uint32_t NewObjectsTime{ 2 };
	uint32_t OldObjectsTime{ 2 };

	std::vector<ProcessColor>& GetProcessColors();
	bool DarkMode{ true };
	bool ThemeAsSystem{ false };

private:
	std::vector<ProcessColor> m_ProcessColors[2];

};

