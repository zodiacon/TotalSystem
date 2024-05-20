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
		_ProcessColorCount
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
		SETTING(ResolveSymbols, 0, SettingType::Bool);
		SETTING(ProcessColorsDark, m_ProcessColors[0], SettingType::Binary);
		SETTING(ProcessColorsLight, m_ProcessColors[1], SettingType::Binary);
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
	DEF_SETTING(ResolveSymbols, bool);

	ProcessColor* GetProcessColors();
	ImVec4 GetRelocatedColor() const;
	static const char* GetColorIndexName(int n);

private:
	std::array<ProcessColor, _ProcessColorCount> m_ProcessColors[2];
	inline static std::array<ImVec4, 2> s_RelocatedColor {
		ImVec4(.3f, .3f, 0, .8f),
		ImVec4(.8f, .8f, 0, .8f),
	};
};

