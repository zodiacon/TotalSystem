#include "pch.h"
#include "WinLowLevel.h"
#include <Psapi.h>
#include <assert.h>

namespace WinLL {
	using namespace std;

	bool Process::Open(uint32_t pid, ProcessAccessMask access) {
		m_hObject.reset(::OpenProcess(access, FALSE, pid));
		return m_hObject.is_valid();
	}

	Process Process::Current() {
		return Process(NtCurrentProcess());
	}

	int32_t Process::GetExitCode() const {
		PROCESS_BASIC_INFORMATION pbi;
		return NT_SUCCESS(NtQueryInformationProcess(Handle(), ProcessBasicInformation, &pbi, sizeof(pbi), nullptr) ? pbi.ExitStatus : -1);
	}

	uint32_t Process::GetId() const {
		return ::GetProcessId(Handle());
	}

	PPEB Process::GetPeb() const {
		PROCESS_BASIC_INFORMATION pbi;
		return NT_SUCCESS(NtQueryInformationProcess(Handle(), ProcessBasicInformation, &pbi, sizeof(pbi), nullptr)) ? pbi.PebBaseAddress : nullptr;
	}

	wstring Process::GetCommandLine() const {
		ULONG len = 1024;
		NTSTATUS status;
		do {
			auto buffer = make_unique<BYTE[]>(len);
			status = NtQueryInformationProcess(Handle(), ProcessCommandLineInformation, buffer.get(), len, nullptr);
			if (NT_SUCCESS(status)) {
				auto str = (PUNICODE_STRING)buffer.get();
				return wstring(str->Buffer, str->Length / sizeof(WCHAR));
			}
			len *= 2;
		} while (status == STATUS_INFO_LENGTH_MISMATCH);
		return L"";
	}

	int32_t Process::GetSessionId() const {
		DWORD session;
		return ::ProcessIdToSessionId(::GetProcessId(m_hObject.get()), &session) ? session : -1;
	}

	wstring Process::GetUserName(uint32_t pid, bool includeDomain) {
		Token token;
		if (!token.Open(TokenAccessMask::Query, pid))
			return L"";
		return token.GetUserName(includeDomain);
	}

	wstring Process::GetImagePath() const {
		BYTE buffer[MAX_PATH * 2];
		if (NT_SUCCESS(NtQueryInformationProcess(Handle(), ProcessImageFileNameWin32, buffer, sizeof(buffer), nullptr))) {
			auto str = (PUNICODE_STRING)buffer;
			return wstring(str->Buffer, str->Length / sizeof(WCHAR));
		}
		return L"";
	}

	wstring Process::GetImageName() const {
		auto full = GetFullImageName();
		if(full.empty())
			return L"";

		auto pos = full.rfind(L'\\');
		return pos == wstring::npos ? full : full.substr(pos + 1);
	}

	PriorityClass Process::GetPriorityClass() const {
		return PriorityClass(::GetPriorityClass(m_hObject.get()));
	}

	bool Process::SetPriorityClass(PriorityClass pc) {
		return ::SetPriorityClass(Handle(), static_cast<DWORD>(pc));
	}

	bool Process::SetBackgroundMode(bool back) {
		ULONG memPri = back ? 1 : 5, ioPri = back ? 0 : 2;
		auto status = NtSetInformationProcess(Handle(), ProcessPagePriority, &memPri, sizeof(memPri));
		status |= NtSetInformationProcess(Handle(), ProcessIoPriority, &ioPri, sizeof(ioPri));
		status |= !::SetPriorityClass(Handle(), back ? IDLE_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS);
		return NT_SUCCESS(status);
	}

	bool Process::Terminate(int32_t exitCode) {
		return NT_SUCCESS(NtTerminateProcess(m_hObject.get(), exitCode));
	}

	bool Process::Suspend() {
		return NT_SUCCESS(::NtSuspendProcess(Handle()));
	}

	bool Process::Resume() {
		return NT_SUCCESS(::NtResumeProcess(Handle()));
	}

	bool Process::IsImmersive() const noexcept {
		return IsWindows8OrGreater() && ::IsImmersiveProcess(Handle()) ? true : false;
	}

	bool Process::IsProtected() const {
		PROCESS_EXTENDED_BASIC_INFORMATION info;
		if (!GetExtendedInfo(&info))
			return false;

		return info.IsProtectedProcess ? true : false;
	}

	bool Process::IsSecure() const {
		PROCESS_EXTENDED_BASIC_INFORMATION info;
		if (!GetExtendedInfo(&info))
			return false;

		return info.IsSecureProcess ? true : false;
	}

	bool Process::IsInJob(HANDLE hJob) const {
		BOOL injob = FALSE;
		::IsProcessInJob(Handle(), hJob, &injob);
		return injob ? true : false;
	}

	bool Process::IsWow64Process() const {
		PROCESS_EXTENDED_BASIC_INFORMATION info;
		if (!GetExtendedInfo(&info))
			return false;

		return info.IsWow64Process ? true : false;
	}

	bool Process::IsManaged() const {
		wil::unique_handle hProcess;
		if (!::DuplicateHandle(::GetCurrentProcess(), Handle(), ::GetCurrentProcess(), hProcess.addressof(),
			PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, 0))
			return false;

		WCHAR filename[MAX_PATH];
		BOOL wow64 = FALSE;
		::IsWow64Process(hProcess.get(), &wow64);
		auto dir = SystemInformation::GetSystemDirectory();
		dir += L"\\mscoree.dll";

		HMODULE hModule[64];
		DWORD needed;
		if (!::EnumProcessModulesEx(hProcess.get(), hModule, sizeof(hModule), &needed, wow64 ? LIST_MODULES_32BIT : LIST_MODULES_ALL))
			return false;

		auto count = (int)min(_countof(hModule), needed / sizeof(HMODULE));

		for (int i = 0; i < count; i++) {
			if (::GetModuleFileNameEx(hProcess.get(), hModule[i], filename, MAX_PATH) == 0)
				continue;
			if (::_wcsicmp(filename, dir.c_str()) == 0)
				return true;
		}
		return false;
	}

	bool Process::GetExtendedInfo(PROCESS_EXTENDED_BASIC_INFORMATION* info) const {
		ULONG len;
		info->Size = sizeof(*info);
		auto status = ::NtQueryInformationProcess(Handle(), ProcessBasicInformation, info, sizeof(*info), &len);
		return NT_SUCCESS(status);
	}

	wstring Process::GetFullImageName() const {
		DWORD size = MAX_PATH;
		WCHAR name[MAX_PATH];
		auto success = ::QueryFullProcessImageName(Handle(), 0, name, &size);
		return success ? std::wstring(name) : L"";
	}

	IntegrityLevel Process::GetIntegrityLevel() const {
		wil::unique_handle hToken;
		if (!::OpenProcessToken(Handle(), TOKEN_QUERY, hToken.addressof()))
			return IntegrityLevel::Error;

		BYTE buffer[256];
		DWORD len;
		if (!::NtQueryInformationToken(hToken.get(), TokenIntegrityLevel, buffer, 256, &len))
			return IntegrityLevel::Error;

		auto integrity = reinterpret_cast<TOKEN_MANDATORY_LABEL*>(buffer);

		auto sid = integrity->Label.Sid;
		return (IntegrityLevel)(*::GetSidSubAuthority(sid, *::GetSidSubAuthorityCount(sid) - 1));
	}

	ProcessProtection Process::GetProtection() const {
		ProcessProtection protection;
		ULONG len;
		auto status = ::NtQueryInformationProcess(Handle(), ProcessProtectionInformation, &protection, sizeof(protection), &len);
		if (!NT_SUCCESS(status))
			return ProcessProtection();
		return protection;
	}

	int Process::GetMemoryPriority() const {
		int priority = -1;
		ULONG len;
		::NtQueryInformationProcess(Handle(), ProcessPagePriority, &priority, sizeof(priority), &len);
		return priority;
	}

	IoPriority Process::GetIoPriority() const {
		auto priority = IoPriority::Unknown;
		ULONG len;
		::NtQueryInformationProcess(Handle(), ProcessIoPriority, &priority, sizeof(priority), &len);
		return priority;
	}

	bool Process::GetProcessPeb(HANDLE hProcess, PPEB peb) {
		PROCESS_BASIC_INFORMATION info;
		if (!NT_SUCCESS(NtQueryInformationProcess(hProcess, ProcessBasicInformation, &info, sizeof(info), nullptr)))
			return false;

		return ::ReadProcessMemory(hProcess, info.PebBaseAddress, peb, sizeof(*peb), nullptr);
	}

	std::wstring Process::GetCurrentDirectory() const {
		std::wstring path;
		PEB peb;
		if(!GetProcessPeb(Handle(), &peb))
			return path;

		RTL_USER_PROCESS_PARAMETERS processParams;
		if (!::ReadProcessMemory(Handle(), peb.ProcessParameters, &processParams, sizeof(processParams), nullptr))
			return path;

		path.resize(processParams.CurrentDirectory.DosPath.Length / sizeof(WCHAR) + 1);
		if (!::ReadProcessMemory(Handle(), processParams.CurrentDirectory.DosPath.Buffer, path.data(), processParams.CurrentDirectory.DosPath.Length, nullptr))
			return L"";

		return path;
	}

	uint64_t Process::GetCycleCount() const {
		PROCESS_CYCLE_TIME_INFORMATION info;
		return NT_SUCCESS(NtQueryInformationProcess(Handle(), ProcessCycleTime, &info, sizeof(info), nullptr)) ? info.AccumulatedCycles : 0;
	}

	vector<pair<wstring, wstring>> Process::GetEnvironment() const {
		vector<pair<wstring, wstring>> env;

		PEB peb;
		if (!GetProcessPeb(Handle(), &peb))
			return env;

		RTL_USER_PROCESS_PARAMETERS processParams;
		if (!::ReadProcessMemory(Handle(), peb.ProcessParameters, &processParams, sizeof(processParams), nullptr))
			return env;

		BYTE buffer[1 << 16];
		int size = sizeof(buffer);
		for (;;) {
			if (::ReadProcessMemory(Handle(), processParams.Environment, buffer, size, nullptr))
				break;
			size -= 1 << 12;
		}

		env.reserve(64);

		for (auto p = (PWSTR)buffer; *p; ) {
			std::pair<std::wstring, std::wstring> var;
			auto equal = wcschr(p, L'=');
			assert(equal);
			if (!equal)
				break;
			*equal = L'\0';
			var.first = p;
			p += ::wcslen(p) + 1;
			var.second = p;
			p += ::wcslen(p) + 1;
			env.push_back(std::move(var));
		}

		return env;
	}

}
