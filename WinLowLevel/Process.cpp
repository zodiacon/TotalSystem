#include "pch.h"
#include "WinLowLevel.h"

namespace WinLL {
	bool Process::Open(uint32_t pid, ProcessAccessMask access) {
		OBJECT_ATTRIBUTES oa = RTL_CONSTANT_OBJECT_ATTRIBUTES(nullptr, 0);
		CLIENT_ID cid = { UlongToHandle(pid) };
		return NT_SUCCESS(::NtOpenProcess(m_hObject.addressof(), access, &oa, &cid));
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

	std::wstring Process::GetCommandLine() const {
		ULONG len = 1024;
		NTSTATUS status;
		do {
			auto buffer = std::make_unique<BYTE[]>(len);
			status = NtQueryInformationProcess(Handle(), ProcessCommandLineInformation, buffer.get(), len, nullptr);
			if (NT_SUCCESS(status)) {
				auto str = (PUNICODE_STRING)buffer.get();
				return std::wstring(str->Buffer, str->Length / sizeof(WCHAR));
			}
			len *= 2;
		} while (status == STATUS_INFO_LENGTH_MISMATCH);
		return L"";
	}

	int32_t Process::GetSessionId() const {
		DWORD session;
		return ::ProcessIdToSessionId(::GetProcessId(m_hObject.get()), &session) ? session : -1;
	}

	std::wstring Process::GetUserName(bool includeDomain) const {
		Token token;
		if (!token.Open(TokenAccessMask::Query))
			return L"";
		return token.GetUserName(includeDomain);
	}

	std::wstring Process::GetAppId() const {
		return std::wstring();
	}

	std::wstring Process::GetImagePath() const {
		BYTE buffer[MAX_PATH * 2];
		if (NT_SUCCESS(NtQueryInformationProcess(Handle(), ProcessImageFileNameWin32, buffer, sizeof(buffer), nullptr))) {
			auto str = (PUNICODE_STRING)buffer;
			return std::wstring(str->Buffer, str->Length / sizeof(WCHAR));
		}
		return L"";
	}

	PriorityClass Process::GetPriorityClass() const {
		return PriorityClass(::GetPriorityClass(m_hObject.get()));
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
}
