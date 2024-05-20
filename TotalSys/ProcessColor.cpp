#include "pch.h"
#include "ProcessColor.h"

ProcessColor::ProcessColor(const ImVec4& defaultColor, const ImVec4& defaultTextColor, bool enabled) : 
	DefaultColor(defaultColor), Color(defaultColor), 
	DefaultTextColor(defaultTextColor), TextColor(defaultTextColor),
	Enabled(enabled) {
}
