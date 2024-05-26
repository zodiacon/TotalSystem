#include "pch.h"
#include "ObjectHelper.h"
#include "HandlesView.h"
#include "FormatHelper.h"
#include <ObjectManager.h>
#include "DriverHelper.h"

using namespace std;
using namespace WinLL;

string ObjectHelper::GetObjectDetails(HandleInfoEx* hi, std::wstring const& type, ProcessManager<>* pm) {
	auto h = DupHandle(ULongToHandle(hi->HandleValue), hi->ProcessId);
	if (!h)
		return "";

	string details;
	if (type == L"Mutant") {
		MUTANT_BASIC_INFORMATION info;
		MUTANT_OWNER_INFORMATION owner;

		if (NT_SUCCESS(NtQueryMutant(h, MutantBasicInformation, &info, sizeof(info), nullptr))
			&& NT_SUCCESS(NtQueryMutant(h, MutantOwnerInformation, &owner, sizeof(owner), nullptr))) {
			details = format("Owner: ({}:{}), Count: {}, Abandoned: {}",
				HandleToULong(owner.ClientId.UniqueProcess), HandleToULong(owner.ClientId.UniqueThread),
				info.CurrentCount, info.AbandonedState ? "True" : "False");
		}
	}
	else if (type == L"Event") {
		EVENT_BASIC_INFORMATION info;
		if (NT_SUCCESS(NtQueryEvent(h, EventBasicInformation, &info, sizeof(info), nullptr))) {
			details = format("Type: {}, Signaled: {}", 
				info.EventType == SynchronizationEvent ? "Synchronization" : "Notification",
				info.EventState ? "True" : "False");
		}
	}
	else if (type == L"Semaphore") {
		SEMAPHORE_BASIC_INFORMATION info;
		if (NT_SUCCESS(NtQuerySemaphore(h, SemaphoreBasicInformation, &info, sizeof(info), nullptr))) {
			details = format("Maximum: {} (0x{:X}), Current: {} (0x{:X})",
				info.MaximumCount, info.MaximumCount, info.CurrentCount, info.CurrentCount);
		}
	}
	else if (type == L"Job") {
		JOBOBJECT_BASIC_ACCOUNTING_INFORMATION info;
		SILOOBJECT_BASIC_INFORMATION silo;
		string ssilo;
		if (NT_SUCCESS(NtQueryInformationJobObject(h, (JOBOBJECTINFOCLASS)JobObjectSiloBasicInformation, &silo, sizeof(silo), nullptr)))
			ssilo = format("({} Silo) ", silo.IsInServerSilo ? "Server" : "App");

		if (NT_SUCCESS(NtQueryInformationJobObject(h, (JOBOBJECTINFOCLASS)JobObjectBasicAccountingInformation, &info, sizeof(info), nullptr))) {
			details = format("{}Processes: {}, Total Processes: {}, CPU Time: {}", ssilo,
				info.ActiveProcesses, info.TotalProcesses,
				FormatHelper::FormatTimeSpan(info.TotalKernelTime.QuadPart + info.TotalUserTime.QuadPart));
		}
	}
	else if (type == L"Process") {
		auto pid = ::GetProcessId(h);
		wstring name, d;
		if (pm) {
			auto p = pm->GetProcessById(pid);
			if (!p) {
				pm->Update();
				p = pm->GetProcessById(pid);
			}
			if (p)
				name = p->GetImageName();
		}
		if (name.empty()) {
			Process p;
			if (p.Open(pid, ProcessAccessMask::QueryLimitedInformation)) {
				name = p.GetImageName();
			}
		}
		KERNEL_USER_TIMES times;
		thread_local static char buffer[256];
		if (NT_SUCCESS(NtQueryInformationProcess(h, ProcessTimes, &times, sizeof(times), nullptr))) {
			sprintf_s(buffer, "PID: %u (%ws) Created: %s Exited: %s", pid, name.c_str(),
				FormatHelper::FormatDateTime(times.CreateTime.QuadPart).c_str(),
				times.ExitTime.QuadPart == 0 ? "(running)" : FormatHelper::FormatDateTime(times.ExitTime.QuadPart).c_str());
		}
		else {
			sprintf_s(buffer, "PID: %u (%ws)", pid, name.c_str());
		}
		details = buffer;
	}
	else if (type == L"Section") {
		SECTION_BASIC_INFORMATION bi;
		if (NT_SUCCESS(NtQuerySection(h, SectionBasicInformation, &bi, sizeof(bi), nullptr))) {
			details = format("Size: 0x{:X} KB, Attributes: 0x{:X} ({})",
				bi.MaximumSize.QuadPart >> 8,
				bi.AllocationAttributes, FormatHelper::SectionAttributesToString(bi.AllocationAttributes));
		}
	}
	else if (type == L"Token") {
		Token token(h);
		details = format("Type: {}, User: {}, Session: {}, Logon Id: 0x{:X}",
			token.GetType() == TokenType::Primary ? "Primary" : "Impersonation",
			FormatHelper::UnicodeToUtf8(token.GetUserName().c_str()),
			token.GetSessionId(), token.GetLogonSessionId());
	}
	::CloseHandle(h);
	return details;
}

bool ObjectHelper::CloseHandle(HandleInfoEx* hi) {
	auto hDup = DupHandle(ULongToHandle(hi->HandleValue), hi->ProcessId, 0, DUPLICATE_CLOSE_SOURCE);
	if (hDup) {
		::CloseHandle(hDup);
		return true;
	}
	return false;
}

HANDLE ObjectHelper::DupHandle(HANDLE h, DWORD pid, ACCESS_MASK access, DWORD flags) {
	auto hDup = WinLLX::ObjectManager::DupHandle(h, pid, access, flags);
	if (!hDup)
		hDup = DriverHelper::DupHandle(h, pid, access, flags);
	return hDup;
}

wstring ObjectHelper::NativePathToDosPath(wstring const& path) {
	static unordered_map<wstring, wstring> drives;
	if (drives.empty()) {
		WCHAR letter[] = L"X:";
		WCHAR path[MAX_PATH];
		for (int c = 2; c < 26; c++) {
			letter[0] = c + 'A';
			if (::QueryDosDevice(letter, path, _countof(path)))
				drives.insert({ path, letter });
		}
	}

	auto bs = path.find(L'\\', 8);
	if (bs == wstring::npos)
		return path;

	if (auto it = drives.find(path.substr(0, bs)); it != drives.end())
		return it->second + path.substr(bs);

	return path;
}

