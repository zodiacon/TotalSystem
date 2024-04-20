#include "pch.h"
#include <subprocesstag.h>
#include "WinLowLevel.h"

namespace WinLL {
	int Thread::GetMemoryPriority() const {
		int priority = -1;
		ULONG len;
		::NtQueryInformationThread(Handle(), ThreadPagePriority, &priority, sizeof(priority), &len);
		return priority;
	}

	IoPriority Thread::GetIoPriority() const {
		IoPriority priority = IoPriority::Unknown;
		ULONG len;
		::NtQueryInformationThread(Handle(), ThreadIoPriority, &priority, sizeof(priority), &len);
		return priority;
	}

	size_t Thread::GetSubProcessTag() const {
		THREAD_BASIC_INFORMATION tbi;
		auto status = ::NtQueryInformationThread(Handle(), ThreadBasicInformation, &tbi, sizeof(tbi), nullptr);
		if (!NT_SUCCESS(status))
			return 0;

		if (tbi.TebBaseAddress == 0)
			return 0;

		bool is64bit = SystemInformation::GetBasicSystemInfo().MaximumAppAddress > (void*)(1LL << 32);
		auto pid = ::GetProcessIdOfThread(Handle());
		Process process;
		if(!process.Open(pid, ProcessAccessMask::QueryLimitedInformation | ProcessAccessMask::VmRead))
			return 0;

		size_t tag = 0;
		if (!is64bit || (process.IsWow64Process() && is64bit)) {
			auto teb = (TEB32*)tbi.TebBaseAddress;
			::ReadProcessMemory(process.Handle(), (BYTE*)teb + offsetof(TEB32, SubProcessTag), &tag, sizeof(ULONG), nullptr);
		}
		else {
			auto teb = (TEB*)tbi.TebBaseAddress;
			::ReadProcessMemory(process.Handle(), (BYTE*)teb + offsetof(TEB, SubProcessTag), &tag, sizeof(tag), nullptr);
		}
		return tag;
	}

	std::wstring Thread::GetServiceNameByTag(uint32_t pid) {
		static auto QueryTagInformation = (PQUERY_TAG_INFORMATION)::GetProcAddress(::GetModuleHandle(L"advapi32"), "I_QueryTagInformation");
		if (QueryTagInformation == nullptr)
			return L"";
		auto tag = GetSubProcessTag();
		if (tag == 0)
			return L"";
		TAG_INFO_NAME_FROM_TAG info = { 0 };
		info.InParams.dwPid = pid;
		info.InParams.dwTag = static_cast<uint32_t>(tag);
		auto err = QueryTagInformation(nullptr, eTagInfoLevelNameFromTag, &info);
		if (err)
			return L"";
		return info.OutParams.pszName;
	}
}
