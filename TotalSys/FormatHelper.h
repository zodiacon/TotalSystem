#pragma once

#include <WinLowLevel.h>

struct FormatHelper abstract final {
	static std::string FormatDateTime(int64_t ft);
	static std::string FormatTimeSpan(int64_t ts);
	static std::wstring FormatNumber(int64_t number);
	static ImVec4 ColorWithAlpha(const ImVec4& color, float alpha);

	static PCSTR VirtualizationStateToString(WinLL::VirtualizationState state);
	static PCSTR IntegrityToString(WinLL::IntegrityLevel level);
	static PCSTR SidNameUseToString(SID_NAME_USE use);
	static std::string ProtectionToString(WinLL::ProcessProtection protection);
	static PCSTR IoPriorityToString(WinLL::IoPriority io);
	static PCSTR PriorityClassToString(WinLL::PriorityClass pc);
	static std::string Format(const char* fmt, ...);
	static std::string GetFolderPath(GUID const& id);
	static std::string UnicodeToUtf8(PCWSTR text);
	static std::string DllCharacteristicsToString(uint16_t dc);
	static std::string HandleAttributesToString(ULONG attributes);
};

