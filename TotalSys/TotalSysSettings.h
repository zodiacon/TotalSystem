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

	int32_t NewObjectsTime{ 2 };
	int32_t OldObjectsTime{ 2 };

	std::vector<ProcessColor>& GetProcessColors();
	ImVec4 GetRelocatedColor() const;

	bool DarkMode{ true };
	bool ThemeAsSystem{ false };
	bool HexIds{ false };

private:
	std::vector<ProcessColor> m_ProcessColors[2];
	ImVec4 m_RelocatedColor[2];
};

