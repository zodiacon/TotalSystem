#include "pch.h"
#include <subprocesstag.h>
#include "WinLowLevel.h"

namespace WinLL {
	bool Thread::Open(uint32_t id, ThreadAccessMask access) {
		m_hObject.reset(::OpenThread(access, FALSE, id));
		return m_hObject != nullptr;
	}

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

	int Thread::GetSuspendCount() const {
		int count = 0;
		NtQueryInformationThread(Handle(), ThreadSuspendCount, &count, sizeof(count), nullptr);
		return count;
	}

	bool Thread::Suspend() {
		return ::SuspendThread(Handle()) != -1;
	}

	bool Thread::Resume() {
		return ::ResumeThread(Handle()) != -1;
	}

	bool Thread::SetPriority(ThreadPriority priority) {
		return ::SetThreadPriority(Handle(), (int)priority);
	}

	bool Thread::Terminate(uint32_t exitCode) {
		return ::TerminateThread(Handle(), exitCode);
	}

	uint32_t Thread::GetId() const {
		return ::GetThreadId(Handle());
	}

	uint32_t Thread::GetProcessId() const {
		return ::GetProcessIdOfThread(Handle());
	}

	ThreadPriority Thread::GetPriority() const {
		return ThreadPriority(::GetThreadPriority(Handle()));
	}

	CpuNumber Thread::GetIdealProcessor() const {
		CpuNumber cpu{ (uint16_t)-1 };
		::GetThreadIdealProcessorEx(Handle(), (PPROCESSOR_NUMBER) & cpu);
		return cpu;
	}

	wstring Thread::GetDescription() const {
		BYTE buffer[256];
		if (NT_SUCCESS(NtQueryInformationThread(Handle(), ThreadNameInformation, buffer, sizeof(buffer), nullptr))) {
			auto name = (PUNICODE_STRING)buffer;
			return wstring(name->Buffer, name->Length / sizeof(WCHAR));
		}
		return L"";
	}

	bool Thread::SetDescription(std::wstring const& desc) {
		return S_OK == ::SetThreadDescription(Handle(), desc.c_str());
	}
}
