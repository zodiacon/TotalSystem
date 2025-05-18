#include "pch.h"
#include "FormatHelper.h"
#include <ShlObj.h>
#include <Shobjidl.h>
#include <ProcessModuleTracker.h>
#include <atlcomcli.h>

using namespace WinLL;
using namespace WinLLX;
using namespace std;

std::string FormatHelper::FormatDateTime(int64_t time) {
	TIME_FIELDS tf;
	RtlTimeToTimeFields((PLARGE_INTEGER)&time, &tf);
	return std::format("{:04}/{:02}/{:02} {:02}:{:02}:{:02}.{:03}",
		tf.Year, tf.Month, tf.Day, tf.Hour, tf.Minute, tf.Second, tf.Milliseconds);
}

std::string FormatHelper::FormatTimeSpan(int64_t ts) {
	TIME_FIELDS tf;
	RtlTimeToElapsedTimeFields((PLARGE_INTEGER)&ts, &tf);
	return std::format("{:2}.{:02}:{:02}:{:02}.{:03}", tf.Day, tf.Hour, tf.Minute, tf.Second, tf.Milliseconds);
}

std::wstring FormatHelper::FormatNumber(int64_t number) {
	static WCHAR sep[4], group[8];
	if (sep[0] == 0) {
		::GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_STHOUSAND, sep, _countof(sep));
		::GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_SGROUPING, group, _countof(group));
	}
	static const NUMBERFMT fmt{ 0, 0, (UINT)_wtoi(group), (PWSTR)L".", sep, 0 };
	WCHAR result[32];
	::GetNumberFormatEx(LOCALE_NAME_USER_DEFAULT, 0, std::to_wstring(number).c_str(), &fmt, result, _countof(result));
	return result;
}

ImVec4 FormatHelper::ColorWithAlpha(const ImVec4& color, float alpha) {
	return ImVec4(color.x, color.y, color.z, alpha);
}

PCSTR FormatHelper::VirtualizationStateToString(VirtualizationState state) {
	switch (state) {
		case VirtualizationState::Disabled: return "Disabled";
		case VirtualizationState::Enabled: return "Enabled";
		case VirtualizationState::NotAllowed: return "Not Allowed";
	}
	return "<Unknown>";
}

PCSTR FormatHelper::IntegrityToString(IntegrityLevel level) {
	switch (level) {
		case IntegrityLevel::High: return "High";
		case IntegrityLevel::Medium: return "Medium";
		case IntegrityLevel::MediumPlus: return "Medium+";
		case IntegrityLevel::Low: return "Low";
		case IntegrityLevel::System: return "System";
		case IntegrityLevel::Untrusted: return "Untrusted";
	}
	return "<Unknown>";
}

PCSTR FormatHelper::SidNameUseToString(SID_NAME_USE use) {
	switch (use) {
		case SidTypeUser: return "User";
		case SidTypeGroup: return "Group";
		case SidTypeDomain: return "Domain";
		case SidTypeAlias: return "Alias";
		case SidTypeWellKnownGroup: return "Well Known Group";
		case SidTypeDeletedAccount: return "Deleted Account";
		case SidTypeInvalid: return "Invalid";
		case SidTypeComputer: return "Computer";
		case SidTypeLabel: return "Label";
		case SidTypeLogonSession: return "Logon Session";
	}
	return "<Unknown>";
}

string FormatHelper::ProtectionToString(WinLL::ProcessProtection protection) {
	if (protection.Level == 0) {
		return "";
	}

	static const PCSTR signer[] = {
		"Authenticode",
		"CodeGen",
		"Antimalware",
		"LSA",
		"Windows",
		"WinTcb",
		"WinSystem",
		"App"
	};
	string result = signer[(int)protection.Signer - 1];
	result += format(" ({}) ", (int)protection.Signer);
	if(protection.Type == 1)
		result += "(PPL)";
	return result;
}

PCSTR FormatHelper::IoPriorityToString(WinLL::IoPriority io) {
	switch (io) {
		case IoPriority::Critical: return "Critical";
		case IoPriority::High: return "High";
		case IoPriority::Low: return "Low";
		case IoPriority::Normal: return "Normal";
		case IoPriority::VeryLow: return "Very Low";
	}
	return "";
}

PCSTR FormatHelper::PriorityClassToString(PriorityClass pc) {
	switch (pc) {
		case PriorityClass::Normal: return "Normal (8)";
		case PriorityClass::AboveNormal: return "Above Normal (10)";
		case PriorityClass::BelowNormal: return "Below Normal (6)";
		case PriorityClass::High: return "High (13)";
		case PriorityClass::Idle: return "Idle (4)";
		case PriorityClass::Realtime: return "Realtime (24)";
	}
	return "";
}

std::string FormatHelper::Format(const char* fmt, ...) {
	char buffer[128];
	va_list args;
	va_start(args, fmt);
	vsprintf_s(buffer, fmt, args);
	va_end(args);
	return buffer;
}

wstring FormatHelper::GetFolderPath(GUID const& id) {
	CComPtr<IKnownFolderManager> spMgr;
	spMgr.CoCreateInstance(CLSID_KnownFolderManager);
	if (spMgr) {
		CComPtr<IKnownFolder> spFolder;
		spMgr->GetFolder(id, &spFolder);
		if (spFolder) {
			PWSTR path;
			if (SUCCEEDED(spFolder->GetPath(KF_FLAG_NO_ALIAS, &path))) {
				wstring result = path;
				::CoTaskMemFree(path);
				return result;
			}
		}
	}
	return L"";
}

std::string FormatHelper::UnicodeToUtf8(PCWSTR text) {
	std::string result;
	auto len = (int)wcslen(text);
	result.resize(len * 2);
	len = ::WideCharToMultiByte(CP_UTF8, WC_NO_BEST_FIT_CHARS, text, len, result.data(), (int)result.length(), nullptr, nullptr);
	if (len) {
		result.resize(len);
		return result;
	}
	return "";
}

std::string FormatHelper::DllCharacteristicsToString(uint16_t dc) {
	std::string result;

	static const struct {
		DllCharacteristics cs;
		PCSTR text;
	} chars[] = {
		{ DllCharacteristics::AppContainer,			"App Container" },
		{ DllCharacteristics::HighEntropyVA,		"High Entropy VA" },
		{ DllCharacteristics::DynamicBase,			"Dynamic Base" },
		{ DllCharacteristics::ForceIntegrity,		"Force Integrity" },
		{ DllCharacteristics::NxCompat,				"NX Compat" },
		{ DllCharacteristics::ControlFlowGuard,		"CFG" },
		{ DllCharacteristics::NoBind,				"No Bind" },
		{ DllCharacteristics::WDMDriver,			"WDM Driver" },
		{ DllCharacteristics::NoIsolation,			"No Isolation" },
		{ DllCharacteristics::TerminalServerAware,	"TS Aware" },
		{ DllCharacteristics::NoSEH,				"No SEH" },
	};

	for (auto& ch : chars) {
		if ((ch.cs & (DllCharacteristics)dc) == ch.cs)
			result += std::string(ch.text) + ", ";
	}

	if (!result.empty())
		result = result.substr(0, result.size() - 2);
	return result;
}

string FormatHelper::HandleAttributesToString(ULONG attributes) {
	if (attributes == 0)
		return "";
	string text;
	if (attributes & OBJ_INHERIT)
		text += "Inherit, ";
	if (attributes & 1)
		text += "Protect, ";
	if (attributes & 4)
		text += "Audit, ";
	return text.empty() ? text : text.substr(0, text.length() - 2) + " (" + std::to_string(attributes) + ")";
}

string FormatHelper::SectionAttributesToString(DWORD value) {
	string text;
	static const struct {
		DWORD attribute;
		PCSTR text;
	} attributes[] = {
		{ SEC_COMMIT, "Commit" },
		{ SEC_RESERVE, "Reserve" },
		{ SEC_IMAGE, "Image" },
		{ SEC_NOCACHE, "No Cache" },
		{ SEC_FILE, "File" },
		{ SEC_WRITECOMBINE, "Write Combine" },
		{ SEC_PROTECTED_IMAGE, "Protected Image" },
		{ SEC_LARGE_PAGES, "Large Pages" },
		{ SEC_IMAGE_NO_EXECUTE, "No Execute" },
	};

	for (auto& item : attributes)
		if (value & item.attribute)
			(text += item.text) += ", ";
	if (text.length() == 0)
		text = "None";
	else
		text = text.substr(0, text.length() - 2);
	return text;
}

