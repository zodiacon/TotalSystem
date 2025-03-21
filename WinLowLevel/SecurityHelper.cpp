#include "pch.h"
#include "SecurityHelper.h"
#include <shellapi.h>
#include <wil\resource.h>
#include "WinLowLevel.h"
#include <assert.h>

namespace WinLL {
	const wstring ifeoKey(L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\");

	using namespace std;

	bool SecurityHelper::IsRunningElevated() {
		static bool runningElevated = false;
		static bool runningElevatedCheck = false;
		if (runningElevatedCheck)
			return runningElevated;

		runningElevatedCheck = true;
		wil::unique_handle hToken;
		if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, hToken.addressof()))
			return false;

		TOKEN_ELEVATION te;
		DWORD len;
		if (::NtQueryInformationToken(hToken.get(), TokenElevation, &te, sizeof(te), &len)) {
			runningElevated = te.TokenIsElevated ? true : false;
		}
		return runningElevated;
	}

	bool SecurityHelper::RunElevated(PCWSTR param, bool ui) {
		WCHAR path[MAX_PATH];
		::GetModuleFileName(nullptr, path, _countof(path));
		SHELLEXECUTEINFO shi = { sizeof(shi) };
		shi.lpFile = path;
		shi.nShow = SW_SHOWDEFAULT;
		shi.lpVerb = L"runas";
		shi.lpParameters = param;
		shi.fMask = (ui ? 0 : (SEE_MASK_FLAG_NO_UI | SEE_MASK_NO_CONSOLE)) | SEE_MASK_NOASYNC | SEE_MASK_NOCLOSEPROCESS;
		auto ok = ::ShellExecuteEx(&shi);
		if (!ok)
			return false;

		DWORD rc = WAIT_OBJECT_0;
		if (!ui) {
			rc = ::WaitForSingleObject(shi.hProcess, 5000);
		}
		::CloseHandle(shi.hProcess);
		return rc == WAIT_OBJECT_0;
	}

	bool SecurityHelper::EnablePrivilege(PCWSTR privName, bool enable) {
		HANDLE hToken;
		if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
			return false;

		bool result = false;
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;
		if (::LookupPrivilegeValue(nullptr, privName,
			&tp.Privileges[0].Luid)) {
			if (::AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp),
				nullptr, nullptr))
				result = ::GetLastError() == ERROR_SUCCESS;
		}
		::CloseHandle(hToken);
		return result;
	}

	wstring SecurityHelper::GetSidFromUser(PCWSTR name) {
		BYTE sid[SECURITY_MAX_SID_SIZE];
		DWORD size = sizeof(sid);
		SID_NAME_USE use;
		WCHAR domain[64];
		DWORD domainSize = _countof(domain);
		if (!::LookupAccountName(nullptr, name, (PSID)sid, &size, domain, &domainSize, &use))
			return L"";

		PWSTR ssid;
		if (::ConvertSidToStringSid((PSID)sid, &ssid)) {
			wstring result(ssid);
			::LocalFree(ssid);
			return result;
		}
		return L"";
	}

	bool SecurityHelper::ReplaceExe(PCWSTR exeToReplace, PCWSTR targetExePath, bool revert) {
		RegistryKey key;
		if (ERROR_SUCCESS != key.Create(HKEY_LOCAL_MACHINE, (ifeoKey + exeToReplace).c_str(), KEY_READ | KEY_WRITE | DELETE))
			return false;

		if (revert)
			return key.DeleteValue(L"Debugger") == ERROR_SUCCESS;

		return key.SetStringValue(L"Debugger", targetExePath, REG_SZ) == ERROR_SUCCESS;
	}

	bool SecurityHelper::IsExeReplaced(PCWSTR exeName) {
		RegistryKey key;
		if (ERROR_SUCCESS != key.Open(HKEY_LOCAL_MACHINE, (ifeoKey + exeName).c_str(), KEY_READ))
			return false;

		WCHAR path[MAX_PATH];
		ULONG chars = _countof(path);
		return key.QueryStringValue(L"Debugger", path, &chars) == ERROR_SUCCESS;

	}
}
