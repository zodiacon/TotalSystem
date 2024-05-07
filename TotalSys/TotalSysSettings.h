#pragma once

#include "ProcessColor.h"
#include "SettingsBase.h"

struct TotalSysSettings : SettingsBase {
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

	BEGIN_SETTINGS(TotalSysSettings)
		SETTING(NewObjectsTime, 2, SettingType::Int32);
		SETTING(OldObjectsTime, 2, SettingType::Int32);
		SETTING(MainWindowPlacement, WINDOWPLACEMENT{}, SettingType::Binary);
		SETTING(AlwaysOnTop, 0, SettingType::Bool);
		SETTING(DarkMode, 1, SettingType::Bool);
		SETTING(ThemeAsSystem, 0, SettingType::Bool);
		SETTING(HexIds, 0, SettingType::Bool);
		SETTING(RelocatedColor, s_RelocatedColor, SettingType::Binary);
		SETTING(ShowLowerPane, 0, SettingType::Bool);
		SETTING(ThreadsWindowOpen, 0, SettingType::Bool);
		SETTING(HandlesWindowOpen, 0, SettingType::Bool);
	END_SETTINGS;

	DEF_SETTING(AlwaysOnTop, bool);
	DEF_SETTING(NewObjectsTime, int);
	DEF_SETTING(OldObjectsTime, int);
	DEF_SETTING(DarkMode, bool);
	DEF_SETTING(ThemeAsSystem, bool);
	DEF_SETTING(HexIds, bool);
	DEF_SETTING(ShowLowerPane, bool);
	DEF_SETTING(ThreadsWindowOpen, bool);
	DEF_SETTING(HandlesWindowOpen, bool);

	std::vector<ProcessColor>& GetProcessColors();
	ImVec4 GetRelocatedColor() const;

private:
	std::vector<ProcessColor> m_ProcessColors[2];
	inline static ImVec4 s_RelocatedColor[2]{
		ImVec4(.3f, .3f, 0, .8f),
		ImVec4(.8f, .8f, 0, .8f),
	};
};

